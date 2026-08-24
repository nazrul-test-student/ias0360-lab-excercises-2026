

#include <cmath> 
#include<iostream>
#include <cstdlib> 
#include <iostream>

#include "pico/stdlib.h"

#include "model.h"
#include "inference.h"
#include "model_settings.h"
#include "mnist_model_data.h"
#include "mnist_image_data.h"

using namespace std; 

#define HALT_CORE_1() while (1) { tight_loop_contents(); }

const uint8_t* test_dataset[] = {
    mnist_image_data_0,
    mnist_image_data_1,
    mnist_image_data_2,
    mnist_image_data_3,
    mnist_image_data_4,
    mnist_image_data_5,
    mnist_image_data_6,
    mnist_image_data_7,
    mnist_image_data_8,
    mnist_image_data_9
};


int count_digits(int number) {
    if (number == 0) {
        return 1; // Special case for 0, which has 1 digit
    }

    number = abs(number); // Handle negative numbers
    return std::floor(std::log10(number)) + 1;
}

void inference_test(void) 
{
    Model ml_model;

    if (!ml_model.setup()) {
        printf("Failed to initialize ML model!\n");
        HALT_CORE_1();
    }
    printf("Model initialized\n");
    
    uint8_t* test_image_input = ml_model.input_data();
    if (test_image_input == nullptr) {
        printf("Cannot set input\n");
        HALT_CORE_1();
    }
    
    int byte_size = ml_model.byte_size();
    if (!byte_size) {
        printf("Byte size not found\n");
        HALT_CORE_1();
    }

    while (true) {
        int random = rand() % 10;
        const uint8_t* sample_data = test_dataset[random];
           
        for (int i=0; i<image_row_size; i++) {
            for (int j=0; j<image_col_size; j++) {
                int num = sample_data[image_col_size*i + j];
                int space = 3 - count_digits(num);
                printf("%d", num);
                for (int i = 0; i < space; ++i) {
                    printf(" ");
                }
            }
            printf("\n");
        }
        
        memcpy(test_image_input, sample_data, byte_size);
        
        int result = ml_model.predict();
        if (result == NAN) {
            printf("Failed to run inference\n");
        } else {
           printf("Actual: %d, Predicted: %d\n", random, result);
        }

        sleep_ms(10000);
    }
    
}