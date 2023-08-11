#include "at_handlers.h"

#include "person_panda.h"

#define MODEL_MAX_SIZE 0x100000 // 每个model的最大size

/* 区分旧版以及新版模型文件， 旧版模型直接通过tflite转
 * 新版本模型通过head+tflie, 其中head=MODEL_MAGIC_NUM | INDEX
 * INDEX定义为1-4为自定义模型，其它为seeed保留模型
 * 目前 0x11 保留位 人体检测模型
 */
#define MODEL_MAGIC_NUM 0x4C485400

static char _at_response_buf[1024];

static struct fdb_kvdb kvdb = {0};
at_config_t at_config = {0};
static SemaphoreHandle_t s_lock = NULL;

static uint32_t partition_start_addr = 0;
static uint32_t partition_size = 0;
static const uint8_t* flash_2_memory_map = NULL;
static el_model_mmap_handler_t mmap_handler = NULL;

static void lock(fdb_db_t db)
{
    xSemaphoreTake(s_lock, portMAX_DELAY);
}

static void unlock(fdb_db_t db)
{
    xSemaphoreGive(s_lock);
}

#define AT_RESPONSE_FORMATE "\r\n{\"type\":\"AT\", \"data\":\"%s\"}\r\n"

#define IMG_PREVIEW_MAX_SIZE 10
#define IMAGE_PREIVEW_ELEMENT_NUM 6
#define IMAGE_PREIVEW_ELEMENT_SIZE 10
#define IMAGE_PREVIEW_FORMATE                                                                     \
    "\r\n{\"type\":\"result\", \"algorithm\":%d, \"model\":%d, \"count\":%d, \"object\":{\"x\": " \
    "[%s],\"y\": [%s],\"w\": [%s],\"h\": [%s],\"target\": [%s],\"confidence\": [%s]}}\r\n"

uint32_t algo_get_models()
{
    uint32_t model = 0; 
    uint8_t index = 0;
    uint32_t model_addr = 0;
    for (uint8_t i = 0; i < 4; i++) {
        model_addr = (intptr_t)flash_2_memory_map + i * MODEL_MAX_SIZE;
        index = i + 1;

        if ((*(uint32_t*)model_addr & 0xFFFFFF00) == MODEL_MAGIC_NUM) {
            index = (*(uint32_t*)model_addr) & 0xFF; // get index form model header
            model_addr += 4;
        }

        if (::tflite::GetModel((void*)model_addr)->version() == TFLITE_SCHEMA_VERSION) {
            model |= 1 << (index); // if model vaild, then set bit
        }
    }
    return model;
}

uint32_t algo_get_model_index(uint8_t model_index)
{
    uint32_t model_addr = 0;
    if (model_index == 0) {
        return (uint32_t)_acperson_panda;
    }
    else {
        if (model_index <= 4) { // custom model
            model_addr = (intptr_t)flash_2_memory_map + (model_index - 1) * MODEL_MAX_SIZE;
        }
        else { // seeed preset model
            model_addr =
                (intptr_t)flash_2_memory_map + (3 - (model_index - 1) % 4) * MODEL_MAX_SIZE;
        }

        // The new version of the model skips the header information
        if ((*(uint32_t*)model_addr & 0xFFFFFF00) == MODEL_MAGIC_NUM) {
            model_addr += 4;
        }
    }
    return model_addr;
}

static void at_reply(const char* fmt, ...)
{
    auto device = Device::get_device();
    auto serial = device->get_serial();
    char print_buf[256] = {0};
    va_list args;
    va_start(args, fmt);
    int r = vsnprintf(print_buf, sizeof(print_buf), fmt, args);
    va_end(args);
    if (r > 0) {
        snprintf(_at_response_buf, sizeof(_at_response_buf), AT_RESPONSE_FORMATE, print_buf);
        serial->write_bytes(_at_response_buf, strlen(_at_response_buf));
    }
}

EL_STA at_id_cmd_handler(int argc, char** argv)
{
    auto* device = Device::get_device();
    at_reply("%08X", device->get_device_id());
    return EL_OK;
}

EL_STA at_reset_cmd_handler(int argc, char** argv)
{
    el_reset();
    return EL_OK;
}

EL_STA at_version_cmd_handler(int argc, char** argv)
{
    at_reply("%s", EL_VERSION);
    return EL_OK;
}

EL_STA at_name_cmd_handler(int argc, char** argv)
{
    auto* device = Device::get_device();
    at_reply("%s", device->get_device_name());
    return EL_OK;
}

EL_STA at_error_cmd_handler(int argc, char** argv)
{
    at_reply("%d", at_config.state);
    return EL_OK;
}

EL_STA at_model_cmd_handler(int argc, char** argv)
{
    if (argc == 0 || argv == nullptr) {
        at_reply("%d", at_config.model);
    }
    else {
        at_config.model = atoi(argv[0]);
        at_reply("OK");
    }
    return EL_OK;
}

EL_STA at_algo_cmd_handler(int argc, char** argv)
{
    if (argc == 0 || argv == nullptr) {
        at_reply("%d", at_config.algo);
    }
    else {
        at_config.algo = atoi(argv[0]);
        at_reply("OK");
    }
    return EL_OK;
}

EL_STA at_invoke_cmd_handler(int argc, char** argv)
{
    if (argc == 0 || argv == nullptr) {
        at_reply("%d", at_config.invoke);
    }
    else {
        at_config.invoke = atoi(argv[0]);
        at_reply("OK");
    }
    return EL_OK;
}

EL_STA at_iou_cmd_handler(int argc, char** argv)
{
    if (argc == 0 || argv == nullptr) {
        at_reply("%d", at_config.iou);
    }
    else {
        int iou = atoi(argv[0]);
        if (iou < 0 || iou > 100) {
            at_reply("ERROR");
            return EL_EINVAL;
        }
        else {
            at_config.iou = iou;
            at_reply("OK");
        }
    }
    return EL_OK;
}

EL_STA at_threshold_cmd_handler(int argc, char** argv)
{
    if (argc == 0 || argv == nullptr) {
        at_reply("%d", at_config.confidence);
    }
    else {
        int threshold = atoi(argv[0]);
        if (threshold < 0 || threshold > 100) {
            at_reply("ERROR");
            return EL_EINVAL;
        }
        else {
            at_config.confidence = threshold;
            at_reply("OK");
        }
    }
    return EL_OK;
}

EL_STA at_cfg_cmd_handler(int argc, char** argv)
{
    at_reply("%d", at_config.rotate);
    return EL_OK;
}

EL_STA at_vmodel_cmd_handler(int argc, char** argv)
{
    char smodel[128] = {0};
    uint32_t model = algo_get_models();
    for (int i = 0; i < 0x20; i++) {
        if (model & (1 << i)) {
            snprintf(smodel + strlen(smodel), sizeof(smodel) - strlen(smodel), "%d,", i);
        }
    }
    at_reply("%s", smodel);
    return EL_OK;
}

EL_STA at_valgo_cmd_handler(int argc, char** argv)
{
    at_reply("%s", "0,");
    return EL_OK;
}

EL_STA at_save_cmd_handler(int argc, char** argv)
{
    struct fdb_blob blob;
    fdb_kv_set_blob(&kvdb, "at_config", fdb_blob_make(&blob, &at_config, sizeof(at_config)));
    at_reply("OK");
    return EL_OK;
}

EL_STA at_pointer_cmd_handler(int argc, char** argv)
{
    char* token;
    if (argv == nullptr || argc == 0) {
        at_reply("%d,%d,%d,%d,%d,%d,%d,%d",
                 at_config.config.pointer.start_x,
                 at_config.config.pointer.start_y,
                 at_config.config.pointer.end_x,
                 at_config.config.pointer.end_y,
                 at_config.config.pointer.center_x,
                 at_config.config.pointer.center_y,
                 at_config.config.pointer.from,
                 at_config.config.pointer.to);
    }
    else {
        at_config.config.pointer.start_x = atoi(argv[0]);
        at_config.config.pointer.start_y = atoi(argv[1]);
        at_config.config.pointer.end_x = atoi(argv[2]);
        at_config.config.pointer.end_y = atoi(argv[3]);
        at_config.config.pointer.center_x = atoi(argv[4]);
        at_config.config.pointer.center_y = atoi(argv[5]);
        at_config.config.pointer.from = atoi(argv[6]);
        at_config.config.pointer.to = atoi(argv[7]);
    }
    return EL_OK;
}
EL_STA at_handler_init(void)
{
    at_config = {0};
    memset(_at_response_buf, 0, sizeof(_at_response_buf));

    el_model_partition_mmap_init(
        &partition_start_addr, &partition_size, &flash_2_memory_map, &mmap_handler);

    printf("partition_start_addr: %08lx\n", partition_start_addr);
    printf("partition_size: %08lx\n", partition_size);
    printf("flash_2_memory_map: %08lx\n", (uint32_t)flash_2_memory_map);

    if (s_lock == NULL) {
        s_lock = xSemaphoreCreateCounting(1, 1);
        assert(s_lock != NULL);
    }

    fdb_err_t result = fdb_kvdb_init(&kvdb, "env", "kvdb0", NULL, NULL);

    if (result != FDB_NO_ERR) {
        printf("kvdb init failed\n");
        return EL_EIO;
    }

    fdb_kvdb_control(&kvdb, FDB_KVDB_CTRL_SET_LOCK, (void*)lock);
    fdb_kvdb_control(&kvdb, FDB_KVDB_CTRL_SET_UNLOCK, (void*)unlock);

    struct fdb_blob blob;

    if (fdb_kv_get_blob(&kvdb, "at_config", fdb_blob_make(&blob, &at_config, sizeof(at_config))) ==
        0) {
            at_config.iou = 45;
            at_config.confidence = 50;
        fdb_kv_set_blob(&kvdb, "at_config", fdb_blob_make(&blob, &at_config, sizeof(at_config)));
    }

    auto repl = ReplServer::get_instance();
    repl->register_cmd(
        AT_ID_CMD, AT_ID_CMD_DESC, AT_ID_CMD_ARGS, nullptr, at_id_cmd_handler, nullptr);
    repl->register_cmd(
        AT_RESET_CMD, AT_RESET_CMD_DESC, AT_RESET_CMD_ARGS, at_reset_cmd_handler, nullptr, nullptr);
    repl->register_cmd(AT_VERSION_CMD,
                       AT_VERSION_CMD_DESC,
                       AT_VERSION_CMD_ARGS,
                       nullptr,
                       at_version_cmd_handler,
                       nullptr);
    repl->register_cmd(
        AT_NAME_CMD, AT_NAME_CMD_DESC, AT_NAME_CMD_ARGS, nullptr, at_name_cmd_handler, nullptr);
    repl->register_cmd(
        AT_ERROR_CMD, AT_ERROR_CMD_DESC, AT_ERROR_CMD_ARGS, nullptr, at_error_cmd_handler, nullptr);
    repl->register_cmd(AT_MODEL_CMD,
                       AT_MODEL_CMD_DESC,
                       AT_MODEL_CMD_ARGS,
                       nullptr,
                       at_model_cmd_handler,
                       at_model_cmd_handler);
    repl->register_cmd(AT_ALGO_CMD,
                       AT_ALGO_CMD_DESC,
                       AT_ALGO_CMD_ARGS,
                       nullptr,
                       at_algo_cmd_handler,
                       at_algo_cmd_handler);
    repl->register_cmd(AT_INVOKE_CMD,
                       AT_INVOKE_CMD_DESC,
                       AT_INVOKE_CMD_ARGS,
                       nullptr,
                       at_invoke_cmd_handler,
                       at_invoke_cmd_handler);
    repl->register_cmd(AT_CONFIDENCE_CMD,
                       AT_CONFIDENCE_CMD_DESC,
                       AT_CONFIDENCE_CMD_ARGS,
                       nullptr,
                       at_threshold_cmd_handler,
                       at_threshold_cmd_handler);
    repl->register_cmd(AT_IOU_CMD,
                       AT_IOU_CMD_DESC,
                       AT_IOU_CMD_ARGS,
                       nullptr,
                       at_iou_cmd_handler,
                       at_iou_cmd_handler);
    repl->register_cmd(AT_CONFIG_CMD,
                       AT_CONFIG_CMD_DESC,
                       AT_CONFIG_CMD_ARGS,
                       nullptr,
                       at_cfg_cmd_handler,
                       nullptr);
    repl->register_cmd(AT_VMODEL_CMD,
                       AT_VMODEL_CMD_DESC,
                       AT_VMODEL_CMD_ARGS,
                       nullptr,
                       at_vmodel_cmd_handler,
                       nullptr);
    repl->register_cmd(
        AT_VALGO_CMD, AT_VALGO_CMD_DESC, AT_VALGO_CMD_ARGS, nullptr, at_valgo_cmd_handler, nullptr);
    repl->register_cmd(
        AT_SAVE_CMD, AT_SAVE_CMD_DESC, AT_SAVE_CMD_ARGS, at_save_cmd_handler, nullptr, nullptr);
    repl->register_cmd(AT_POINTER_CMD,
                       AT_POINTER_CMD_DESC,
                       AT_POINTER_CMD_ARGS,
                       nullptr,
                       at_pointer_cmd_handler,
                       at_pointer_cmd_handler);

    repl->init();
    at_reply("OK");
    return EL_OK;
}

int algo_yolo_get_preview(YOLO* algo, char* preview, uint16_t max_length)
{
    memset(preview, 0, max_length);
    auto _object_detection_list = algo->get_results();
    uint16_t index = 0;
    // 获取目前结果集长度
    uint16_t size = std::distance(_object_detection_list.begin(), _object_detection_list.end());

    if (size == 0) {
        return 1;
    }

    // 输入preview最多能有多少element
    uint16_t available_size = (max_length - sizeof(IMAGE_PREVIEW_FORMATE)) /
                              (IMAGE_PREIVEW_ELEMENT_SIZE * IMAGE_PREIVEW_ELEMENT_NUM);

    if (available_size < 1) {
        return 1;
    }

    std::string element[IMAGE_PREIVEW_ELEMENT_NUM];

    // 生成element
    for (auto it = _object_detection_list.begin(); it != _object_detection_list.end(); ++it) {
        if (index == 0) {
            element[0] = std::to_string(it->x);
            element[1] = std::to_string(it->y);
            element[2] = std::to_string(it->w);
            element[3] = std::to_string(it->h);
            element[4] = std::to_string(it->target);
            element[5] = std::to_string(it->score);
        }
        else {
            element[0] = element[0] + "," + std::to_string(it->x);
            element[1] = element[1] + "," + std::to_string(it->y);
            element[2] = element[2] + "," + std::to_string(it->w);
            element[3] = element[3] + "," + std::to_string(it->h);
            element[4] = element[4] + "," + std::to_string(it->target);
            element[5] = element[5] + "," + std::to_string(it->score);
        }
        index++;
        // 如果超过最大的可预览长度 则退出
        if (index > IMG_PREVIEW_MAX_SIZE || index > available_size) {
            break;
        }
    }

    // 规格化preview
    snprintf(preview,
             max_length,
             IMAGE_PREVIEW_FORMATE,
             at_config.algo,
             at_config.model,
             size,
             element[0].c_str(),
             element[1].c_str(),
             element[2].c_str(),
             element[3].c_str(),
             element[4].c_str(),
             element[5].c_str());

    return 0;
}


int algo_fomo_get_preview(FOMO* algo, char* preview, uint16_t max_length)
{
    memset(preview, 0, max_length);
    auto _object_detection_list = algo->get_results();
    uint16_t index = 0;
    // 获取目前结果集长度
    uint16_t size = std::distance(_object_detection_list.begin(), _object_detection_list.end());

    if (size == 0) {
        return 1;
    }

    // 输入preview最多能有多少element
    uint16_t available_size = (max_length - sizeof(IMAGE_PREVIEW_FORMATE)) /
                              (IMAGE_PREIVEW_ELEMENT_SIZE * IMAGE_PREIVEW_ELEMENT_NUM);

    if (available_size < 1) {
        return 1;
    }

    std::string element[IMAGE_PREIVEW_ELEMENT_NUM];

    // 生成element
    for (auto it = _object_detection_list.begin(); it != _object_detection_list.end(); ++it) {
        if (index == 0) {
            element[0] = std::to_string(it->x);
            element[1] = std::to_string(it->y);
            element[2] = std::to_string(it->w);
            element[3] = std::to_string(it->h);
            element[4] = std::to_string(it->target);
            element[5] = std::to_string(it->score);
        }
        else {
            element[0] = element[0] + "," + std::to_string(it->x);
            element[1] = element[1] + "," + std::to_string(it->y);
            element[2] = element[2] + "," + std::to_string(it->w);
            element[3] = element[3] + "," + std::to_string(it->h);
            element[4] = element[4] + "," + std::to_string(it->target);
            element[5] = element[5] + "," + std::to_string(it->score);
        }
        index++;
        // 如果超过最大的可预览长度 则退出
        if (index > IMG_PREVIEW_MAX_SIZE || index > available_size) {
            break;
        }
    }

    // 规格化preview
    snprintf(preview,
             max_length,
             IMAGE_PREVIEW_FORMATE,
             at_config.algo,
             at_config.model,
             size,
             element[0].c_str(),
             element[1].c_str(),
             element[2].c_str(),
             element[3].c_str(),
             element[4].c_str(),
             element[5].c_str());

    return 0;
}
