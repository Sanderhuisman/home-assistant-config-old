import appdaemon.plugins.hass.hassapi as hass
import datetime


class DockerMonitor(hass.Hass):
    def initialize(self):
        self.listen_event_handle_list = []
        self.listen_state_handle_list = []

        self.notifiers = self.args.get('notifiers', [])

        self.listen_event_handle_list.append(self.listen_event(
            self.event_callback, event='docker_container_event'))

    def event_callback(self, event_name, data, kwargs):
        if event_name == 'docker_container_event':
            # self.log("Event {} fired with data {}".format(event_name, data), level = "DEBUG")

            if data['Status'] in ['start']:
                message = "Container \"{}\" started".format(
                    data['Container'].replace('_', ' '))
                self._notify(message)
            elif data['Status'] in ['die']:
                message = "Container \"{}\" died".format(
                    data['Container'].replace('_', ' '))
                self._notify(message)

            # data['Container']
            # data['Image']
            # data['Status']
            # data['Id']

    def _notify(self, message):
        for notifier in self.notifiers:
            self.notify(message, name=notifier)

    def terminate(self):
        for listen_event_handle in self.listen_event_handle_list:
            self.cancel_listen_event(listen_event_handle)

        for listen_state_handle in self.listen_state_handle_list:
            self.cancel_listen_state(listen_state_handle)
