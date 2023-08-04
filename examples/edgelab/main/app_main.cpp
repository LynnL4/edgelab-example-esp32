
#include <algorithm>
#include <cstring>
#include <iomanip>
#include <sstream>
#include <string>

#include "at_handlers.h"
#include "edgelab.h"
#include "el_device_esp.h"

void task_invoke(void* arg)
{
    auto* device = Device::get_device();
    auto* camera = device->get_camera();
    camera->init(240, 240);

    auto* engine = new InferenceEngine<EngineName::TFLite>();

    if(at_config.algo == 0)
    {

    }


    while (1) {
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

    while (1) {
        repl->loop(serial->get_char());
    }
}
