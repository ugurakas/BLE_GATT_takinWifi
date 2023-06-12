#include <stdio.h>
#include "esp_bt.h"
#include "esp_bt_main.h"
#include "esp_gap_ble_api.h"
#include "esp_gatts_api.h"
#include "esp_bt_device.h"

#define GATTS_TAG           "GATTS_DEMO"
#define PROFILE_NUM         1
#define PROFILE_APP_IDX     0
#define SVC_INST_ID         0

#define WIFI_ID_CHAR_UUID   ESP_BT_UUID_EIR_DECLARE(0xEEEE)
#define PASSWORD_CHAR_UUID  ESP_BT_UUID_EIR_DECLARE(0xDDDD)

#define PROVİSİON_PROFİLE 2
#define WIFI_ID_CHAR_INDEX  0
#define PASSWORD_CHAR_INDEX 1

static esp_gatt_char_prop_t wifi_id_permission = ESP_GATT_CHAR_PROP_BIT_READ | ESP_GATT_CHAR_PROP_BIT_WRITE;
static esp_gatt_char_prop_t password_permission = ESP_GATT_CHAR_PROP_BIT_READ | ESP_GATT_CHAR_PROP_BIT_WRITE;

static uint8_t wifi_id_value[32] = {0};
static uint8_t password_value[32] = {0};

static esp_attr_value_t wifi_id_attr_value = {
    .attr_max_len = sizeof(wifi_id_value),
    .attr_len     = sizeof(wifi_id_value),
    .attr_value   = wifi_id_value,
};

static esp_attr_value_t password_attr_value = {
    .attr_max_len = sizeof(password_value),
    .attr_len     = sizeof(password_value),
    .attr_value   = password_value,
};

static const esp_gatts_attr_db_t gatt_db[WIFI_ID_CHAR_INDEX + PASSWORD_CHAR_INDEX] = {
    [WIFI_ID_CHAR_INDEX] = {
        .att_desc = {
            .uuid_length = ESP_UUID_LEN_16,
            .uuid_p       = (uint8_t *)&WIFI_ID_CHAR_UUID,
            .perm         = ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
            .max_length   = sizeof(wifi_id_value),
            .length       = sizeof(wifi_id_value),
            .value        = wifi_id_value,
        },
    },
    [PASSWORD_CHAR_INDEX] = {
        .att_desc = {
            .uuid_length = ESP_UUID_LEN_16,
            .uuid_p       = (uint8_t *)&PASSWORD_CHAR_UUID,
            .perm         = ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
            .max_length   = sizeof(password_value),
            .length       = sizeof(password_value),
            .value        = password_value,
        },
    },
};

static esp_ble_adv_params_t adv_params = {
    .adv_int_min        = 0x20,
    .adv_int_max        = 0x40,
    .adv_type           = ADV_TYPE_NONCONN_IND,
    .own_addr_type      = BLE_ADDR_TYPE_PUBLIC,
    .channel_map        = ADV_CHNL_ALL,
    .adv_filter_policy  = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
};

static esp_ble_adv_data_t adv_data = {
    .include_name = true,
    .include_txpower = false,
    .min_interval = 0x20,
    .max_interval = 0x40,
    .appearance = 0x00,
    .manufacturer_len = 0,
    .p_manufacturer_data = NULL,
    .service_data_len = 0,
    .p_service_data = NULL,
    .service_uuid_len = 0,
    .p_service_uuid = NULL,
    .flag = 0x6,
};

static esp_ble_adv_data_t scan_rsp_data = {
    .include_name = true,
    .include_txpower = false,
    .min_interval = 0x20,
    .max_interval = 0x40,
    .appearance = 0x00,
    .manufacturer_len = 0,
    .p_manufacturer_data = NULL,
    .service_data_len = 0,
    .p_service_data = NULL,
    .service_uuid_len = 0,
    .p_service_uuid = NULL,
    .flag = 0x6,
};

static esp_gatts_cb_t gatts_cb;

static void gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param)
{
    switch (event) {
        case ESP_GAP_BLE_ADV_DATA_SET_COMPLETE_EVT:
            esp_ble_gap_start_advertising(&adv_params);
            break;

        case ESP_GAP_BLE_SEC_REQ_EVT:
            esp_ble_gap_security_rsp(param->ble_security.ble_req.bd_addr, true);
            break;

        default:
            break;
    }
}

static void gatts_profile_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if,
                                        esp_ble_gatts_cb_param_t *param)
{
    switch (event) {
        case ESP_GATTS_REG_EVT:
            esp_ble_gap_set_device_name("GATT Server");
            esp_ble_gap_config_adv_data(&adv_data);
            esp_ble_gap_config_adv_data(&scan_rsp_data);
            break;

        case ESP_GATTS_CONNECT_EVT:
            break;

        case ESP_GATTS_DISCONNECT_EVT:
            break;

        case ESP_GATTS_READ_EVT:
            if (param->read.handle == gatt_db[WIFI_ID_CHAR_INDEX].att_desc.handle) {
                esp_gatts_send_response(gatts_if, param->read.conn_id, param->read.trans_id, ESP_GATT_OK, &wifi_id_attr_value);
            } else if (param->read.handle == gatt_db[PASSWORD_CHAR_INDEX].att_desc.handle) {
                esp_gatts_send_response(gatts_if, param->read.conn_id, param->read.trans_id, ESP_GATT_OK, &password_attr_value);
            }
            break;

        case ESP_GATTS_WRITE_EVT:
            if (param->write.handle == gatt_db[WIFI_ID_CHAR_INDEX].att_desc.handle) {
                memcpy(wifi_id_value, param->write.value, param->write.len);
                esp_gatts_send_response(gatts_if, param->write.conn_id, param->write.trans_id, ESP_GATT_OK, NULL);
                // Handle WiFi ID value
            } else if (param->write.handle == gatt_db[PASSWORD_CHAR_INDEX].att_desc.handle) {
                memcpy(password_value, param->write.value, param->write.len);
                esp_gatts_send_response(gatts_if, param->write.conn_id, param->write.trans_id, ESP_GATT_OK, NULL);
                // Handle password value
            }
            break;

        default:
            break;
    }
}

void gatts_task(void *param)
{
    esp_err_t ret;

    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    ret = esp_bt_controller_init(&bt_cfg);
    if (ret != ESP_OK) {
        printf("Failed to initialize BT controller\n");
        vTaskDelete(NULL);
    }

    ret = esp_bt_controller_enable(ESP_BT_MODE_BLE);
    if (ret != ESP_OK) {
        printf("Failed to enable BT controller\n");
        vTaskDelete(NULL);
    }

    ret = esp_bluedroid_init();
    if (ret != ESP_OK) {
        printf("Failed to initialize Bluedroid\n");
        vTaskDelete(NULL);
    }

    ret = esp_bluedroid_enable();
    if (ret != ESP_OK) {
        printf("Failed to enable Bluedroid\n");
        vTaskDelete(NULL);
    }

    ret = esp_ble_gatts_register_callback(gatts_profile_event_handler);
    if (ret != ESP_OK) {
        printf("Failed to register GATT server callback\n");
        vTaskDelete(NULL);
    }

    ret = esp_ble_gap_register_callback(gap_event_handler);
    if (ret != ESP_OK) {
        printf("Failed to register GAP callback\n");
        vTaskDelete(NULL);
    }

    ret = esp_ble_gatts_app_register(PROFILE_APP_IDX);
    if (ret != ESP_OK) {
        printf("Failed to register GATT application\n");
        vTaskDelete(NULL);
    }

    while (1) {
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

void app_main()
{
    xTaskCreate(gatts_task, "gatts_task", 4096, NULL, 5, NULL);
}
