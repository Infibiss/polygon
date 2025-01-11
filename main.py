import cv2
import numpy as np
from matplotlib import pyplot as plt
from scipy.ndimage import binary_opening, binary_closing, binary_erosion

# Загружаем изображения
image = cv2.imread('cats.jpg')
image_rgb = cv2.cvtColor(image, cv2.COLOR_BGR2RGB) # В формат RGB так как он cv2 по умолчанию в BGR
image_hsv = cv2.cvtColor(image_rgb, cv2.COLOR_RGB2HSV) # В HSV формат

# Функция для обработки каждого сегмента
def process_segment(segment, tolerance):
    # Определяем угловые пиксели для фона
    corner_pixels = np.concatenate([
        segment[:20, :20].reshape(-1, 3),  # Верхний левый
        segment[:20, -20:].reshape(-1, 3), # Верхний правый
        segment[-20:, :20].reshape(-1, 3), # Нижний левый
        segment[-20:, -20:].reshape(-1, 3) # Нижний правый
    ])
    # Средний цвет фона
    mean_bg_color = np.mean(corner_pixels, axis=0)
    h_bg, s_bg, v_bg = mean_bg_color

    # Создаем маску по HSV с некоторым порогом
    mask = ((np.abs(segment[:, :, 0] - h_bg) < tolerance) &
            (np.abs(segment[:, :, 1] - s_bg) < tolerance) &
            (np.abs(segment[:, :, 2] - v_bg) < tolerance))
    mask = mask.astype(np.uint8) * 255

    # Очищаем маску через бинарные операции
    binary_mask = mask > 0
    # Бинарное морфологическое открытие для закрытия дыр в изображении
    # 1) Эрозия - Уменьшает белые области, съедая края объектов
    # 2) Дилатация - Расширяет оставшиеся белые области, восстанавливая часть потерянных пикселей после эрозии
    binary_mask = binary_opening(binary_mask, structure=np.ones((20, 20)))

    # binary_mask = binary_erosion(binary_mask, structure=np.ones((10, 10)))
    # binary_mask = binary_closing(binary_mask, structure=np.ones((5, 5)))

    mask_cleaned = (binary_mask * 255).astype(np.uint8)

    # Инвертируем маску
    mask_inv = 255 - mask_cleaned

    # Применяем маски
    segment_rgb = cv2.cvtColor(segment, cv2.COLOR_HSV2RGB)
    result = segment_rgb.copy()
    result[mask_inv == 0] = [0, 0, 0] # Удаляем фона
    return result, mask_inv

# Разделяем изображения на 4 части по котам
h, w = image_hsv.shape[:2]
half_h, half_w = h // 2, w // 2

segments = [
    image_hsv[:half_h, :half_w], # Верхний левый
    image_hsv[:half_h, half_w:], # Верхний правый
    image_hsv[half_h:, :half_w], # Нижний левый
    image_hsv[half_h:, half_w:]  # Нижний правый
]

# Применяем алгоритм к каждой части
results = []
masks = []
tolerances = [35, 85, 60, 100] # Пороги для каждого кота

for i, segment in enumerate(segments):
    result, mask = process_segment(segment, tolerances[i])
    results.append(result)
    masks.append(mask)

# Объединяем сегменты обратно
top_row = np.hstack((results[0], results[1]))
bottom_row = np.hstack((results[2], results[3]))
final_image = np.vstack((top_row, bottom_row))

plt.figure(figsize=(10, 10))
plt.imshow(final_image)
plt.title("Background Removed (4 Segments)")
plt.axis('off')
plt.show()