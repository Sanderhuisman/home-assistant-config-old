import appdaemon.plugins.hass.hassapi as hass
import datetime


class TransmissionMonitor(hass.Hass):
    def initialize(self):
        self.listen_event_handle_list = []
        self.listen_state_handle_list = []

        self.notifiers = self.args.get('notifiers', [])

        self.listen_event_handle_list.append(
            self.listen_event(self.event_callback, event='transmission_started_torrent'))
        self.listen_event_handle_list.append(
            self.listen_event(self.event_callback, event='transmission_downloaded_torrent'))

    def event_callback(self, event_name, data, kwargs):
        self.notify("Event {} fired with data".format(event_name))

        for notifier in self.notifiers:
            self.notify("Transmission event: {} {}".format(
                event_name, data['name']), name=notifier)

    def terminate(self):
        for listen_event_handle in self.listen_event_handle_list:
            self.cancel_listen_event(listen_event_handle)

        for listen_state_handle in self.listen_state_handle_list:
            self.cancel_listen_state(listen_state_handle)
