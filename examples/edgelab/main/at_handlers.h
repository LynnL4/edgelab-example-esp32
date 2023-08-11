#ifndef AT_HANDLERS_H_
#define AT_HANDLERS_H_

#include "edgelab.h"

#define AT_CMD "AT"
#define AT_CMD_LEN (sizeof(AT_CMD) - 1)
#define AT_CMD_DESC "AT command"
#define AT_CMD_ARGS ""

#define AT_NAME_CMD "NAME"
#define AT_NAME_CMD_LEN (sizeof(AT_NAME_CMD) - 1)
#define AT_NAME_CMD_DESC "Get device name"
#define AT_NAME_CMD_ARGS ""

#define AT_RESET_CMD "RST"
#define AT_RESET_CMD_LEN (sizeof(AT_RESET_CMD) - 1)
#define AT_RESET_CMD_DESC "Reset device"
#define AT_RESET_CMD_ARGS ""

#define AT_VERSION_CMD "VER"
#define AT_VERSION_CMD_LEN (sizeof(AT_VERSION_CMD) - 1)
#define AT_VERSION_CMD_DESC "Get firmware version"
#define AT_VERSION_CMD_ARGS ""

#define AT_ID_CMD "ID"
#define AT_ID_CMD_LEN (sizeof(AT_ID_CMD) - 1)
#define AT_ID_CMD_DESC "Get device ID"
#define AT_ID_CMD_ARGS ""

#define AT_ERROR_CMD "ERR"
#define AT_ERROR_CMD_LEN (sizeof(AT_ERROR_CMD) - 1)
#define AT_ERROR_CMD_DESC "Get device error"
#define AT_ERROR_CMD_ARGS ""

#define AT_ALGO_CMD "ALGO"
#define AT_ALGO_CMD_LEN (sizeof(AT_ALGO_CMD) - 1)
#define AT_ALGO_CMD_DESC "Get algorithm"
#define AT_ALGO_CMD_ARGS "<algo>"

#define AT_MODEL_CMD "MODEL"
#define AT_MODEL_CMD_LEN (sizeof(AT_MODEL_CMD) - 1)
#define AT_MODEL_CMD_DESC "Get model"
#define AT_MODEL_CMD_ARGS "<model>"

#define AT_VMODEL_CMD "VMODEL"
#define AT_VMODEL_CMD_LEN (sizeof(AT_VMODEL_CMD) - 1)
#define AT_VMODEL_CMD_DESC "Get exsiting models"
#define AT_VMODEL_CMD_ARGS ""

#define AT_VALGO_CMD "VALGO"
#define AT_VALGO_CMD_LEN (sizeof(AT_VALGO_CMD) - 1)
#define AT_VALGO_CMD_DESC "Get exsiting algorithms"
#define AT_VALGO_CMD_ARGS ""

#define AT_CONFIDENCE_CMD "CONF"
#define AT_CONFIDENCE_CMD_LEN (sizeof(AT_CONFIDENCE_CMD) - 1)
#define AT_CONFIDENCE_CMD_DESC "Get confidence"
#define AT_CONFIDENCE_CMD_ARGS "<confidence>"

#define AT_CONFIG_CMD "CFG"
#define AT_CONFIG_CMD_LEN (sizeof(AT_CONFIG_CMD) - 1)
#define AT_CONFIG_CMD_DESC "Get configuration"
#define AT_CONFIG_CMD_ARGS "<config>"

#define AT_IOU_CMD "IOU"
#define AT_IOU_CMD_LEN (sizeof(AT_IOU_CMD) - 1)
#define AT_IOU_CMD_DESC "Get IOU"
#define AT_IOU_CMD_ARGS "<iou>"

#define AT_INVOKE_CMD "INVOKE"
#define AT_INVOKE_CMD_LEN (sizeof(AT_INVOKE_CMD) - 1)
#define AT_INVOKE_CMD_DESC "Invoke algorithm"
#define AT_INVOKE_CMD_ARGS ""

#define AT_SAVE_CMD "SAVE"
#define AT_SAVE_CMD_LEN (sizeof(AT_SAVE_CMD) - 1)
#define AT_SAVE_CMD_DESC "Save Configuration"
#define AT_SAVE_CMD_ARGS ""

#define AT_POINTER_CMD "POINT"
#define AT_POINTER_CMD_LEN (sizeof(AT_POINTER_CMD) - 1)
#define AT_POINTER_CMD_DESC "point"
#define AT_POINTER_CMD_ARGS "<start_x>,<start_y> <end_x> <end_y> <center_x> <center_y> <from> <to>"

typedef struct EL_ATTR_PACKED {
    uint8_t algo;
    uint8_t model;
    uint8_t state;
    uint8_t rotate;
    int32_t invoke;
    uint8_t confidence;
    uint8_t iou;
    struct {
        struct {
            uint16_t start_x;
            uint16_t start_y;
            uint16_t end_x;
            uint16_t end_y;
            uint16_t center_x;
            uint16_t center_y;
            uint32_t from; // keep 3 point after decimal point   1.234 multiply 1000
            uint32_t to;   // keep 3 point after decimal point   1.234 multiply 1000
        } pointer;
    } config;
} at_config_t;

extern at_config_t at_config;
EL_STA at_handler_init(void);
extern uint32_t algo_get_model_index(uint8_t model_index);
int algo_yolo_get_preview(YOLO* algo, char* preview, uint16_t max_length);
int algo_fomo_get_preview(FOMO* algo, char* preview, uint16_t max_length);
#endif /* AT_HANDLERS_H_ */