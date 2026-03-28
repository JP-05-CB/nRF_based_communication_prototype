#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
#include <driver/i2s.h> 

// ================== PINS & HARDWARE ==================
#define MIC_PIN           34
#define BUTTON_PIN        13
#define I2S_BCLK_PIN      26
#define I2S_LRC_PIN       25
#define I2S_DOUT_PIN      22

RF24 radio(4, 5); 
const byte address[6] = "00001";

// Shared Memory Buffer (Saves RAM!)
const int MAX_PACKETS = 1000;
uint8_t shared_audio_buffer[MAX_PACKETS][32]; 
int packet_count = 0;

// State Tracking
bool is_recording = false;
bool is_receiving = false;
unsigned long last_receive_time = 0;

// ================== ADPCM TABLES & MATH ==================
const int step_size_table[89] = {
  7, 8, 9, 10, 11, 12, 13, 14, 16, 17, 19, 21, 23, 25, 28, 31, 34, 37, 41, 45, 
  50, 55, 60, 66, 73, 80, 88, 97, 107, 118, 130, 143, 157, 173, 190, 209, 230, 
  253, 279, 307, 337, 371, 408, 449, 494, 544, 598, 658, 724, 796, 876, 963, 
  1060, 1166, 1282, 1411, 1552, 1707, 1878, 2066, 2272, 2499, 2749, 3024, 3327, 
  3660, 4026, 4428, 4871, 5358, 5894, 6484, 7132, 7845, 8630, 9493, 10442, 
  11487, 12635, 13899, 15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794, 32767
};
const int index_table[8] = {-1, -1, -1, -1, 2, 4, 6, 8};

int encoder_pred = 0; int encoder_index = 0; 
int decoder_pred = 0; int decoder_index = 0;

uint8_t adpcm_encode_sample(int16_t sample) {
    int step = step_size_table[encoder_index];
    int diff = sample - encoder_pred;
    int sign = (diff < 0) ? 8 : 0;
    if (sign) diff = -diff;
    int delta = 0;
    int diff_q = step >> 3;

    if (diff >= step) { delta |= 4; diff -= step; diff_q += step; }
    step >>= 1;
    if (diff >= step) { delta |= 2; diff -= step; diff_q += step; }
    step >>= 1;
    if (diff >= step) { delta |= 1; diff_q += step; }

    if (sign) encoder_pred -= diff_q;
    else      encoder_pred += diff_q;

    if (encoder_pred > 32767) encoder_pred = 32767;
    else if (encoder_pred < -32768) encoder_pred = -32768;

    encoder_index += index_table[delta];
    if (encoder_index < 0) encoder_index = 0;
    if (encoder_index > 88) encoder_index = 88;

    return (sign | delta);
}

int16_t adpcm_decode_sample(uint8_t nibble) {
    int step = step_size_table[decoder_index];
    int diff_q = step >> 3;

    if (nibble & 4) diff_q += step;
    if (nibble & 2) diff_q += (step >> 1);
    if (nibble & 1) diff_q += (step >> 2);

    if (nibble & 8) decoder_pred -= diff_q;
    else            decoder_pred += diff_q;

    if (decoder_pred > 32767) decoder_pred = 32767;
    else if (decoder_pred < -32768) decoder_pred = -32768;

    decoder_index += index_table[nibble & 7];
    if (decoder_index < 0) decoder_index = 0;
    if (decoder_index > 88) decoder_index = 88;

    return decoder_pred;
}

// ================== SETUP ==================
void setup() {
  Serial.begin(115200);
  pinMode(BUTTON_PIN, INPUT_PULLUP); 

  // Setup Radio
  radio.begin();
  radio.openWritingPipe(address);
  radio.openReadingPipe(0, address);
  radio.setPALevel(RF24_PA_HIGH);
  radio.startListening(); // Default state is listening

  // Setup I2S Amplifier
  i2s_config_t i2s_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
    .sample_rate = 8000,                            
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,   
    .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,    
    .communication_format = I2S_COMM_FORMAT_I2S_MSB,
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count = 8,
    .dma_buf_len = 64,
    .use_apll = false
  };
  
  i2s_pin_config_t pin_config = {
    .bck_io_num = I2S_BCLK_PIN,
    .ws_io_num = I2S_LRC_PIN,
    .data_out_num = I2S_DOUT_PIN,
    .data_in_num = I2S_PIN_NO_CHANGE
  };
  
  i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);
  i2s_set_pin(I2S_NUM_0, &pin_config);

  Serial.println("Walkie-Talkie Ready! Listening for incoming, or hold button to talk.");
}

// ================== MAIN LOOP ==================
void loop() {
  bool button_pressed = (digitalRead(BUTTON_PIN) == LOW);

  // --- STATE 1: TRANSMITTING (Button Held) ---
  if (button_pressed && !is_receiving) {
    if (!is_recording) {
      is_recording = true;
      radio.stopListening(); // Become deaf while talking
      packet_count = 0;
      encoder_pred = 0; encoder_index = 0;
      Serial.println("🔴 Recording...");
    }

    if (packet_count < MAX_PACKETS) {
      int16_t temp_buffer[64];
      for(int i = 0; i < 64; i++) {
        temp_buffer[i] = map(analogRead(MIC_PIN), 0, 4095, -32768, 32767); 
        delayMicroseconds(125); 
      }
      
      for(int i = 0; i < 32; i++) {
        uint8_t code1 = adpcm_encode_sample(temp_buffer[i*2]);
        uint8_t code2 = adpcm_encode_sample(temp_buffer[i*2 + 1]);
        shared_audio_buffer[packet_count][i] = (code1 << 4) | code2; 
      }
      packet_count++;
    }
  } 
  // --- STATE 2: FINISHED TRANSMITTING (Button Released) ---
  else if (!button_pressed && is_recording) {
    is_recording = false;
    Serial.print("⏹️ Stopped. Transmitting ");
    Serial.print(packet_count);
    Serial.println(" packets...");

    for(int i = 0; i < packet_count; i++) {
      radio.write(&shared_audio_buffer[i], 32);
      delay(5); 
    }
    
    Serial.println("✅ TX Complete. Back to Listening.");
    radio.startListening(); // Open ears again
  } 
  
  // --- STATE 3: LISTENING & RECEIVING (Button Not Held) ---
  else if (!button_pressed) {
    
    if (radio.available()) {
      if (!is_receiving) {
        is_receiving = true;
        packet_count = 0;
        decoder_pred = 0; decoder_index = 0;
        Serial.println("📥 Receiving radio data...");
      }

      if (packet_count < MAX_PACKETS) {
        radio.read(&shared_audio_buffer[packet_count], 32);
        packet_count++;
      } else {
        uint8_t dummy[32]; radio.read(&dummy, 32); 
      }
      last_receive_time = millis(); 
    }

    // --- STATE 4: PLAYBACK (Silence detected on radio) ---
    if (is_receiving && (millis() - last_receive_time > 1500)) {
      Serial.print("🔊 Playing "); Serial.print(packet_count); Serial.println(" packets...");

      for (int p = 0; p < packet_count; p++) {
        int16_t stereo_chunk[128]; 
        for (int i = 0; i < 32; i++) {
          uint8_t byte_in = shared_audio_buffer[p][i];
          uint8_t nibble1 = (byte_in >> 4) & 0x0F;
          uint8_t nibble2 = byte_in & 0x0F;

          int16_t sample1 = adpcm_decode_sample(nibble1);
          int16_t sample2 = adpcm_decode_sample(nibble2);

          stereo_chunk[i*4] = sample1;     
          stereo_chunk[i*4 + 1] = sample1; 
          stereo_chunk[i*4 + 2] = sample2; 
          stereo_chunk[i*4 + 3] = sample2; 
        }
        size_t bytes_written;
        i2s_write(I2S_NUM_0, stereo_chunk, sizeof(stereo_chunk), &bytes_written, portMAX_DELAY);
      }
      Serial.println("🏁 Playback finished. Listening...");
      is_receiving = false; 
    }
  }
}