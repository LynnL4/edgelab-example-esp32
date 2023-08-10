
#include <algorithm>
#include <cstring>
#include <iomanip>
#include <sstream>
#include <string>

#include "at_handlers.h"
#include "edgelab.h"
#include "el_device_esp.h"
static char result[1024];

void task_invoke(void* arg)
{
    auto* device = Device::get_device();
    auto* camera = device->get_camera();
    camera->init(240, 240);

    auto* engine = new InferenceEngine<EngineName::TFLite>();

    static auto* tensor_arena{heap_caps_malloc(1024 * 1024, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)};
    memset(tensor_arena, 0, 1024 * 1024);
    auto ret = engine->init(tensor_arena, 1024 * 1024);

    auto model = algo_get_model_index(at_config.model);
    if (engine->load_model((const void*)model, 1024 * 1024) != EL_OK) {
        printf("load model failed");
        return;
    }

    el_shape_t output_shape = engine->get_output_shape(0);
    static auto* algo = new YOLO(engine);

    while (1) {
        vTaskDelay(10 / portTICK_PERIOD_MS);
        if (at_config.invoke != 0) {
            at_config.invoke--;
            el_img_t img;
            camera->start_stream();
            camera->get_frame(&img);
            algo->run(&img);
            uint32_t preprocess_time = algo->get_preprocess_time();
            uint32_t run_time = algo->get_run_time();
            uint32_t postprocess_time = algo->get_postprocess_time();
            auto* current_img = &img;
            auto size{current_img->width * current_img->height * 3};
            auto rgb_img{el_img_t{.data = new uint8_t[size]{},
                                  .size = size,
                                  .width = current_img->width,
                                  .height = current_img->height,
                                  .format = EL_PIXEL_FORMAT_RGB888,
                                  .rotate = current_img->rotate}};
            ret = rgb_to_rgb2(current_img, &rgb_img);
            if (ret == EL_OK) {
                auto jpeg_img{el_img_t{.data = new uint8_t[size]{},
                                       .size = size,
                                       .width = rgb_img.width,
                                       .height = rgb_img.height,
                                       .format = EL_PIXEL_FORMAT_JPEG,
                                       .rotate = rgb_img.rotate}};
                ret = rgb_to_jpeg(&rgb_img, &jpeg_img);
                if (ret == EL_OK) {
                    delete[] rgb_img.data;
                    auto buffer{new char[32 * 1024]{}};
                    el_base64_encode(jpeg_img.data, jpeg_img.size, buffer);
                    delete[] jpeg_img.data;
                    printf(
                        "\r\n{\"type\":\"preview\", \"img\":\"%s\", \"preprocess_time\":%ld, "
                        "\"run_time\":%ld, "
                        "\"postprocess_time\":%ld}\r\n",
                        buffer,
                        preprocess_time,
                        run_time,
                        postprocess_time);
                    delete[] buffer;
                }
                vTaskDelay(10 / portTICK_PERIOD_MS);
                algo_yolo_get_preview(algo, result, sizeof(result));
                if (strlen(result) > 0)
                    printf("%s", result);
            }

            camera->stop_stream();
        }
    }
}

extern "C" void app_main(void)
{
    // fetch hardware resource
    auto* device = Device::get_device();
    auto* camera = device->get_camera();
    auto* serial = device->get_serial();
    auto* repl = ReplServer::get_instance();

    serial->init();

    at_handler_init();

    // create task
    xTaskCreate(task_invoke, "task_invoke", 1024 * 48, NULL, 5, NULL);

    while (1) {
        repl->loop(serial->get_char());
    }
}
