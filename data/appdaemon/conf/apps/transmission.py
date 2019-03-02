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
        self.log("Event {} fired with data {}".format(event_name, data))

        if event_name == 'transmission_started_torrent':
            self._notify("Transmission torrent {} started".format(data['name']))
        elif event_name == 'transmission_downloaded_torrent':
            self._notify("Transmission torrent {} finished".format(data['name']))


    def _notify(self, message):
        self.log("Sending notification: {}".format(message))
        for notifier in self.notifiers:
            self.notify(message, name=notifier)

    def terminate(self):
        for listen_event_handle in self.listen_event_handle_list:
            self.cancel_listen_event(listen_event_handle)

        for listen_state_handle in self.listen_state_handle_list:
            self.cancel_listen_state(listen_state_handle)
