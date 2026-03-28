clc; clear; close all;

% ================== AUDIO INPUT ==================
Fs = 8000;
nbits = 16;                                     %if we need to change the values , then the step table also gets changed 
nchannels = 2;

recorder1 = audiorecorder(Fs, nbits, nchannels);

disp("Begin speaking...");
recDuration = 10;
recordblocking(recorder1, recDuration);

disp("End of recording...");
x = getaudiodata(recorder1, 'double');
soundsc(x, Fs);   % auto-scales volume (recommended)

x = x/max(abs(x));

y_in = int16(round(x*32767));
% ================== IMA ADPCM TABLES ==================
step_size_table = [7, 8, 9, 10, 11, 12, 13, 14, 16, 17, 19, 21, 23, 25, 28, ...
                   31, 34, 37, 41, 45, 50, 55, 60, 66, 73, 80, 88, 97, 107, ...
                   118, 130, 143, 157, 173, 190, 209, 230, 253, 279, 307, ...
                   337, 371, 408, 449, 494, 544, 598, 658, 724, 796, 876, ...
                   963, 1060, 1166, 1282, 1411, 1552, 1707, 1878, 2066, ...
                   2272, 2499, 2749, 3024, 3327, 3660, 4026, 4428, 4871, ...
                   5358, 5894, 6484, 7132, 7845, 8630, 9493, 10442, 11487, ...
                   12635, 13899, 15289, 16818, 18500, 20350, 22385, 24623, ...
                   27086, 29794, 32767];

index_table = [-1 -1 -1 -1 2 4 6 8];

% ================== ENCODER ==================
N = length(y_in);
code = zeros(N,1,'uint8');          % 4-bit ADPCM symbols

pred  = 0;
index = 1;

for i = 1:N
    
    step = step_size_table(index);
    diff = y_in(i) - pred;

    % Sign extraction
    if diff < 0
        sign = 8;                   % 1000 (negative)
        mag  = -diff;
    else
        sign = 0;                   % 0000 (positive)
        mag  = diff;
    end

    % Delta quantization (3 bits)
    delta = min(floor(mag / step), 7);
    code(i) = uint8(sign + delta);

    % Internal reconstruction (encoder sync)
    % Replace the old diff_q in your ENCODER loop with this:
    diff_q = floor(step / 8);
    %MID - RISER because reconstructed value to be in the center of the quantization range to minimize error
    if bitand(delta, 4), diff_q = diff_q + floor(step); end
    if bitand(delta, 2), diff_q = diff_q + floor(step / 2); end
    if bitand(delta, 1), diff_q = diff_q + floor(step / 4); end

    if sign == 8
        pred = pred - diff_q;
    else
        pred = pred + diff_q;
    end

    pred = max(-32768, min(32767, pred));

    % Step size adaptation
    index = index + index_table(delta + 1);
    index = max(1, min(89, index));
end

% ================== PACK 4-BIT CODES INTO BYTES ==================
packed_len = ceil(N/2);
packed = zeros(packed_len,1,'uint8');

k = 1;
for i = 1:2:N
    if i+1 <= N
        packed(k) = bitor(bitshift(code(i),4), code(i+1));
    else
        packed(k) = bitshift(code(i),4);  % $$last nibble padded
    end
    k = k + 1;
end

% ================== ADD NOISE  ==================
BER = 7e-2;%1e-3; % 0.1% bit error rate 
packed_noisy = packed; 
for i = 1:length(packed) 
    for b = 0:7 
        if rand < BER 
            packed_noisy(i) = bitxor(packed_noisy(i), bitshift(1,b));%$$ 
        end 
    end 
end

% ================== WRITE BINARY FILE ==================
fid = fopen('adpcm_code.bin','wb');
fwrite(fid, packed_noisy, 'uint8');
fclose(fid);

% ================== SAVE METADATA ==================
save('adpcm_meta.mat','Fs','N');

disp('IMA-ADPCM encoding complete.');
disp('Binary file saved as: adpcm_code.bin');
disp('Metadata saved as: adpcm_meta.mat');