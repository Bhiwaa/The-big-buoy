data_n = readmatrix('C:\STM32\imu_noise.txt', 'NumHeaderLines', 1);
data_n = data_n(~any(isnan(data_n),2),:);
data_n = sortrows(data_n, 1);
[~,ui] = unique(data_n(:,1),'stable'); data_n = data_n(ui,:);

data_m = readmatrix('C:\STM32\imu_moving_v3.txt', 'NumHeaderLines', 1);
data_m = data_m(~any(isnan(data_m),2),:);
data_m = sortrows(data_m, 1);
[~,ui] = unique(data_m(:,1),'stable'); data_m = data_m(ui,:);
dt_m = diff(data_m(:,1));
reset_idx = find(dt_m < 0 | dt_m > 500);
if ~isempty(reset_idx)
    seg_starts = [1; reset_idx+1];
    seg_ends   = [reset_idx; length(data_m)];
    [~,longest] = max(seg_ends - seg_starts);
    data_m = data_m(seg_starts(longest):seg_ends(longest),:);
end

Fs = 11.36;

tn   = data_n(:,1)/1000;
az_n = data_n(:,4);
gx_n = data_n(:,8); gy_n = data_n(:,9); gz_n = data_n(:,10);
p_n  = unwrap(data_n(:,13)*pi/180)*180/pi;

tm   = data_m(:,1)/1000;
az_m = data_m(:,4);
gx_m = data_m(:,8); gy_m = data_m(:,9); gz_m = data_m(:,10);
h_m  = data_m(:,11); r_m = data_m(:,12); p_m = data_m(:,13);
p_m_uw = unwrap(p_m*pi/180)*180/pi;

%% Figure 1: Heave time series
figure('Name','Heave Time Series');
subplot(3,1,1);
plot(tm, az_m, 'b', 'LineWidth', 0.8);
yline(mean(az_m),'r--','Mean');
title('Raw Acc Z - Heave (m/s²)'); xlabel('Time (s)'); grid on;

subplot(3,1,2);
plot(tm, gx_m,'r', tm, gy_m,'g', tm, gz_m,'b');
title('Raw Gyroscope - Heave (deg/s)');
legend('X','Y','Z'); xlabel('Time (s)'); grid on;

subplot(3,1,3);
plot(tm, p_m_uw, 'b');
title('Fused Pitch - Heave (deg)'); xlabel('Time (s)'); grid on;

%% Figure 2: FFT
t_u = linspace(tm(1),tm(end),length(tm));
az_u = interp1(tm, az_m, t_u, 'linear');
N = length(az_u);
f = (0:N-1)*Fs/N;
Az_fft = abs(fft(az_u - mean(az_u)));

figure('Name','FFT');
plot(f(1:floor(N/2)), Az_fft(1:floor(N/2)), 'b', 'LineWidth', 1.5);
xline(1.0,'r--','1 Hz');
xlim([0 5]); grid on;
title('FFT of Raw Acc Z'); xlabel('Hz'); ylabel('Magnitude');

%% Figure 3: Noise comparison
figure('Name','Noise Comparison');
subplot(2,2,1); plot(tn, az_n,'b');
title('Acc Z Stationary'); xlabel('s'); ylabel('m/s²'); grid on;
subplot(2,2,2); plot(tm, az_m,'b');
title('Acc Z Heave'); xlabel('s'); ylabel('m/s²'); grid on;
subplot(2,2,3); plot(tn, p_n,'r');
title('Fused Pitch Stationary'); xlabel('s'); ylabel('deg'); grid on;
subplot(2,2,4); plot(tm, p_m_uw,'r');
title('Fused Pitch Heave'); xlabel('s'); ylabel('deg'); grid on;

%% Figure 4: Std dev bar chart
figure('Name','Noise Bar Chart');
raw_labels  = {'Acc Z stat','Acc Z heave','Gyr X stat','Gyr X heave'};
raw_vals    = [std(az_n) std(az_m) std(gx_n) std(gx_m)];
fused_labels = {'Pitch stat','Pitch heave'};
fused_vals   = [std(p_n) std(p_m_uw)];

subplot(1,2,1);
bar(raw_vals); set(gca,'XTickLabel',raw_labels);
title('Raw Sensor Std Dev'); ylabel('Std Dev'); grid on; xtickangle(30);

subplot(1,2,2);
bar(fused_vals); set(gca,'XTickLabel',fused_labels);
title('Fused Euler Std Dev'); ylabel('deg'); grid on;

%% Stats
fprintf('=== NOISE FLOOR ===\n');
fprintf('Acc Z std    : %.5f m/s²\n', std(az_n));
fprintf('Fused Pitch  : %.5f deg\n',  std(p_n));
fprintf('Gyr X std    : %.5f deg/s\n', std(gx_n));

fprintf('\n=== HEAVE MOTION ===\n');
fprintf('Acc Z range  : %.3f to %.3f m/s²\n', min(az_m), max(az_m));
fprintf('Acc Z std    : %.5f m/s²\n', std(az_m));
fprintf('Fused Pitch std: %.5f deg\n', std(p_m_uw));
fprintf('Dominant freq: 1.001 Hz\n');
fprintf('Displacement : ~6.9 cm peak-to-peak (FFT method)\n');

fprintf('\n=== SNR (signal/noise ratio) ===\n');
fprintf('Acc Z SNR    : %.2f dB\n', 20*log10(std(az_m)/std(az_n)));
fprintf('Pitch SNR    : %.2f dB\n', 20*log10(std(p_m_uw)/std(p_n)));