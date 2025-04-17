import numpy as np
from scipy.io.wavfile import write
from pydub import AudioSegment

# 音阶频率（Hz）
frequencies = {
    'gong': [130.81, 261.63, 523.25],
    'shang': [146.83, 293.66, 587.33],
    'jiao': [164.81, 329.63, 659.26],
    'qingjiao': [174.61, 349.23, 698.46],
    'zheng': [196.00, 392.00, 784.00],
    'yu': [220.00, 440.00, 880.00],
    'biangong': [246.94, 493.88, 987.77]
}

# 采样率
sample_rate = 44100

# 每个音频的时长
duration = 1.3  # 秒

# 衰减率
decay_rate = 2.0 # 值越大衰减越快

# 生成带衰减的正弦波
def generate_sine_wave_with_decay(frequency, duration, sample_rate, decay_rate):
    t = np.linspace(0, duration, int(sample_rate * duration), endpoint=False)
    # 生成正弦波
    sine_wave = np.sin(2 * np.pi * frequency * t)
    # 生成指数衰减包络
    decay_envelope = np.exp(-decay_rate * t)
    # 应用衰减
    return sine_wave * decay_envelope

# 保存音频到wav文件
def save_wave(filename, audio_data, sample_rate):
    # 归一化到[-1, 1]
    normalized_audio = audio_data / np.max(np.abs(audio_data))
    # 需要转换为16位整数格式
    audio_data_int16 = np.int16(normalized_audio * 32767)
    write(filename, sample_rate, audio_data_int16)

# 生成所有音阶的音频
audio_files = []

# 映射关系
chinese_to_western = {
    'gong': 'C',
    'shang': 'D',
    'jiao': 'E',
    'qingjiao': 'F',
    'zheng': 'G',
    'yu': 'A',
    'biangong': 'B'
}
octave_map = {
    'low': 3,
    'mid': 4,
    'high': 5
}

for note, freqs in frequencies.items():
    for idx, freq in enumerate(freqs):
        octave_desc = ['low', 'mid', 'high'][idx]
        western_note = chinese_to_western.get(note, 'Unknown') # 获取西洋音名
        octave_num = octave_map.get(octave_desc, '') # 获取八度数字

        # 格式化文件名：中文音阶名_西洋音名八度.wav
        filename = f"{note}_{western_note}{octave_num}.wav"

        decaying_sine_wave = generate_sine_wave_with_decay(freq, duration, sample_rate, decay_rate)
        save_wave(filename, decaying_sine_wave, sample_rate)
        audio_files.append(filename)

# 定义音名顺序用于排序
note_order = ['C', 'D', 'E', 'F', 'G', 'A', 'B']

# 定义排序函数
def get_sort_key(filename):
    # 从 "gong_C3.wav" 提取 "C3"
    parts = filename.split('_')
    western_part = parts[-1].split('.')[0] # 'C3'
    note = ''.join(filter(str.isalpha, western_part)) # 'C'
    octave = int(''.join(filter(str.isdigit, western_part))) # 3
    try:
        note_index = note_order.index(note)
    except ValueError:
        note_index = -1 # 处理未知音名
    return (octave, note_index)

# 对文件列表进行排序
audio_files.sort(key=get_sort_key)

# 将所有生成的音频文件按排序后的顺序合并成一个音频
print("按照音高顺序合并音频...")
combined = AudioSegment.empty() # 创建一个空的AudioSegment
if audio_files: # 确保列表不为空
    combined = AudioSegment.from_wav(audio_files[0])
    for file in audio_files[1:]:
        next_audio = AudioSegment.from_wav(file)
        combined += next_audio
else:
    print("没有找到要合并的音频文件。")


# 导出合并后的音频文件
combined.export("combined_sorted_decaying_sine_waves.wav", format="wav")

print("按音高排序并带衰减的音频文件已生成并保存为 'combined_sorted_decaying_sine_waves.wav'")
