import appdaemon.plugins.hass.hassapi as hass
import datetime


class Sander(hass.Hass):
    def initialize(self):
        self.listen_state_handle_list = []
        self.home = []

        self.device_trackers = self.args.get('device_tracker', [])
        self.light_sensor = self.args.get('light_sensor')
        self.threshold = float(self.args.get('threshold', 50))
        self.hysteresis = float(self.args.get('hysteresis', 10))
        self.notifiers = self.args.get('notifiers', [])

        irradiance = self.get_state(self.light_sensor)
        self.log("irradiance {}".format(irradiance))
        self.dark = None
        try:
            if float(irradiance) <= self.threshold:
                self.dark = True
            else:
                self.dark = False
        except ValueError:
            self.log("Not a float")

        for tracker in self.device_trackers:
            state = self.get_state(tracker)
            self.log("{} is {}".format(tracker, state))
            if state == 'home':
                self.home.append(tracker)

            self.listen_state_handle_list.append(
                self.listen_state(self.state_change, tracker))

        self.listen_state_handle_list.append(
            self.listen_state(self.state_change, self.light_sensor))

    def state_change(self, entity, attribute, old, new, kwargs):
        changed = False
        if entity == self.light_sensor:
            if self.dark is None and new != None and new != "":
                self.dark = self.threshold >= float(new)
            else:
                self.log("{} changed to {}".format(
                    self.friendly_name(entity), new))
                if self.dark == True and (self.threshold - self.hysteresis) < float(new):
                    self.dark = False
                    changed = True
                    self.log("It got light {} -> {}".format(entity, float(new)))

                    self._notify("It got light {} -> {}".format(entity, float(new)))
                elif self.dark == False and (self.threshold + self.hysteresis) >= float(new):
                    self.dark = True
                    changed = True
                    self.log("It got dark {} -> {}".format(entity, float(new)))

                    self._notify("It got dark {} -> {}".format(entity, float(new)))
        elif entity in self.device_trackers:
            # attributes might have changed while state didn't
            if old != new:
                if new == 'home':
                    self.log("{} got home".format(entity))
                    self.home.append(entity)
                    changed = True

                    self._notify("{} got home".format(entity))
                else:
                    if entity in self.home:
                        self.log("{} left :(".format(entity))
                        self.home.remove(entity)
                        changed = True

                        self._notify("{} left".format(entity))

                self.log("{} changed to {}".format(
                    self.friendly_name(entity), new))

        if changed:
            if self.dark == True and self.home:
                self.log("Turn light on")

                self._notify("Turn light on")
            else:
                self.log("Turn light off")

                self._notify("Turn light off")

    def _notify(self, message):
        for notifier in self.notifiers:
            self.notify(message, name=notifier)
        pass

    def terminate(self):
        for listen_state_handle in self.listen_state_handle_list:
            self.cancel_listen_state(listen_state_handle)
