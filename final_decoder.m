clc; clear; close all;

% ================== LOAD METADATA ==================
load('adpcm_meta.mat');   % loads Fs and N (original number of samples)

% ================== READ BINARY FILE ==================
fid = fopen('adpcm_code.bin','rb');
packed = fread(fid,'uint8');
fclose(fid);

% ================== UNPACK BYTES → 4-BIT CODES ==================
code = zeros(N,1,'uint8');

k = 1;
for i = 1:length(packed)
    if k <= N
        code(k) = bitshift(packed(i), -4);      % upper nibble
        k = k + 1;
    end
    if k <= N
        code(k) = bitand(packed(i), 15);        % lower nibble
        k = k + 1;
    end
end

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

% ================== DECODER ==================
% ================== REFINED DECODER ==================
y_decoded = zeros(N, 1);
pred = 0;
index = 1;

for i = 1:N
    step = step_size_table(index);
    nibble = code(i);
    
    % Standard IMA ADPCM reconstruction logic
    diff_q = floor(step / 8);
    if bitand(nibble, 4), diff_q = diff_q + floor(step); end
    if bitand(nibble, 2), diff_q = diff_q + floor(step / 2); end
    if bitand(nibble, 1), diff_q = diff_q + floor(step / 4); end
    
    if bitand(nibble, 8)
        pred = pred - diff_q;
    else
        pred = pred + diff_q;
    end
    
    % Clamp and update
    pred = max(-32768, min(32767, pred));
    y_decoded(i) = pred;
    
    index = index + index_table(bitand(nibble, 7) + 1);
    index = max(1, min(89, index));
end

% CRITICAL: Scale for output
y_out = y_decoded / 32768; 
audiowrite('decoded_adpcm.wav', y_out, Fs);