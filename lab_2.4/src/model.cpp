#include "tensorflow/lite/micro/tflite_bridge/micro_error_reporter.h"
#include "tensorflow/lite/schema/schema_generated.h"
#include "tensorflow/lite/micro/micro_mutable_op_resolver.h"
#include "tensorflow/lite/micro/micro_interpreter.h"

#include "model.h"
#include "model_settings.h"
// TODO: 4. Import your model data



Model::Model() :
    model(nullptr),
    interpreter(nullptr),
    input(nullptr),
    error_reporter(nullptr)
{
}

Model::~Model()
{
    if (interpreter != NULL) {
        delete interpreter;
        interpreter = NULL;
    }
    if (model != NULL) {
        delete model;
        model = NULL;
    }
    if (input != NULL) {
        delete input;
        input = NULL;
    }
    if (error_reporter != NULL) {
        delete error_reporter;
        error_reporter = NULL;
    }
}

int Model::setup() 
{
  static tflite::MicroErrorReporter micro_error_reporter;
  error_reporter = &micro_error_reporter;

  model = tflite::GetModel(/* TODO: 5. Load your model. */);
  if (model->version() != TFLITE_SCHEMA_VERSION) {
    TF_LITE_REPORT_ERROR(error_reporter,
                         "Model provided is schema version %d not equal "
                         "to supported version %d.",
                         model->version(), TFLITE_SCHEMA_VERSION);
    return 0;
  }

  static tflite::MicroMutableOpResolver</* TODO: 6. Change operations resolver size. */> micro_op_resolver;
  // TODO: 7. Add operations according to your model.
  
  static uint8_t tensor_arena[arena_size];
  // Build an interpreter to run the model with.
  // NOLINTNEXTLINE(runtime-global-variables)
  static tflite::MicroInterpreter static_interpreter(
      model, micro_op_resolver, tensor_arena, arena_size);
  interpreter = &static_interpreter;

  // TODO: 8. Allocate tensor
  

  // Get information about the memory area to use for the model's input.
  input = interpreter->input(0);

  return 1;
}

uint8_t* Model::input_data() {
  if (input == nullptr) {
    return nullptr;
  }
  return input->data.uint8;
}

int Model::byte_size() {
  if (input == nullptr) {
    return 0;
  }
  return input->bytes;
}

int Model::predict()
{
  printf("Invocation started\n");
  // TODO: 9. Run invoke inference, if error, return -1

  printf("Invocation finished\n");

  TfLiteTensor* output = interpreter->output(0);

  int result = -1;
  // TODO: 10. Return an index of the output neuron, which has maximum probability.
  
  return result;
}