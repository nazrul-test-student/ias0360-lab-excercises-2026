#ifndef TFLITE_INFERENCE_TEST_MODEL_SETTINGS_H_
#define TFLITE_INFERENCE_TEST_MODEL_SETTINGS_H_

constexpr int kNumCols = 96;
constexpr int kNumRows = 96;
constexpr int kNumChannels = 1;

constexpr int kMaxImageSize = kNumCols * kNumRows * kNumChannels;

constexpr int kCategoryCount = 2;
constexpr int kPersonIndex = 1;
constexpr int kNotAPersonIndex = 0;
extern const char* kCategoryLabels[kCategoryCount];

constexpr int image_col_size = 28;
constexpr int image_row_size = 28;

constexpr int arena_size = 10 * 1024; // TODO: 3. Edit this for your own model.
#endif  // TFLITE_INFERENCE_TEST_MODEL_SETTINGS_H_
