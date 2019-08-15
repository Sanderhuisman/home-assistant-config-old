import appdaemon.plugins.hass.hassapi as hass

class SystemStartup(hass.Hass):
    def initialize(self):
        self.listen_event(self.ha_event, "plugin_started")

    def ha_event(self, event_name, data, kwargs):
        self.call_service("mqtt/publish", topic= "cmnd/sonoffs/POWER", payload=" ")

        self.call_service("mqtt/publish", topic= "esp/state", payload="sync")
