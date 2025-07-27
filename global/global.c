#include "global/global.h"
int dev_panel_get_twin_result(DevPanel *panel, const char *deviceID, const char *property, char **out_value, char **out_datatype) { return 0; }
int dev_panel_write_device(DevPanel *panel, const char *method, const char *deviceID, const char *property, const char *data) { return 0; }
int dev_panel_get_device_method(DevPanel *panel, const char *deviceID, char ***out_method_map, int *out_method_count, char ***out_property_map, int *out_property_count) { return 0; }
int dev_panel_get_device(DevPanel *panel, const char *deviceID, DeviceInstance *out) { return 0; }
int dev_panel_get_model(DevPanel *panel, const char *modelID, DeviceModel *out) { return 0; }