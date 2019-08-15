import appdaemon.plugins.hass.hassapi as hass
import datetime


class Home(hass.Hass):
    def initialize(self):
        self.listen_state_handle_list = []
        self.home = []

        self.media_players = self.args.get('media_players', [])
        self.lights = self.args.get('lights', [])


        self.device_trackers = self.args.get('device_tracker', [])
        self.light_sensor = self.args.get('light_sensor')
        self.threshold = float(self.args.get('threshold', 50))
        self.hysteresis = float(self.args.get('hysteresis', 10))
        self.notifiers = self.args.get('notifiers', [])

        irradiance = self.get_state(self.light_sensor)
        self.log("irradiance {}".format(irradiance))
        self.dark = None
        self.On = False
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

        for media_player in self.media_players:
            self.listen_state_handle_list.append(
                self.listen_state(self.state_change, media_player))

    def state_change(self, entity, attribute, old, new, kwargs):
        if entity == self.light_sensor:
            #self._notify("Irradiance {}  {}".format(entity, new))

            if self.dark is None and new != None and new != "":
                self.dark = self.threshold >= float(new)
            else:
                # self.log("changed to {}".format(new))
                if self.dark == True and (self.threshold + self.hysteresis) < float(new):
                    self.dark = False

                    # self._notify("It got light")
                elif self.dark == False and (self.threshold - self.hysteresis) > float(new):
                    self.dark = True

                    # self._notify("It got dark")
        elif entity in self.device_trackers:
            # attributes might have changed while state didn't
            state = self.get_state(entity, attribute="all")
            self.log("Tracker {} update ({}); State: {}".format(entity, attribute, state))

            if old != new:
                if new == 'home':
                    self.log("{} got home ({})".format(entity, attribute))
                    self.home.append(entity)

                    self._notify("{} got home".format(entity))
                else:
                    if entity in self.home:
                        self.log("{} left :( ({})".format(entity, attribute))
                        self.home.remove(entity)

                        self._notify("{} left".format(entity))

                self.log("{} changed to {}".format(
                    self.friendly_name(entity), new))
        elif entity in self.media_players:
            self.log("Player {} update ({})".format(entity, attribute))
            self._notify("Player {} update ({})".format(entity, attribute))
            state = self.get_state(entity, attribute="all")
            self.log("State: {}".format(state))

        if self.dark == True and self.home and self.On == False:
            self.On = True
            # self.log("Turn light on")

            # self._notify("Turn light on")
        elif (self.dark == False or not self.home) and self.On == True:
            self.On = False
            # self.log("Turn light off")

            # self._notify("Turn light off")

    def _notify(self, message):
        self.log("Notification: {}".format(message))
        for notifier in self.notifiers:
            self.notify(message, name=notifier)

    def terminate(self):
        for listen_state_handle in self.listen_state_handle_list:
            self.cancel_listen_state(listen_state_handle)
