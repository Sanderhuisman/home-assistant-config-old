import appdaemon.plugins.hass.hassapi as hass

TIMER_ON = 'timer_on'
TIMER_OFF = 'timer_off'


class DeviceMonitor(hass.Hass):
    def initialize(self):
        self.timer_handle_list = {}
        self.listen_event_handle_list = {}
        self.listen_state_handle_list = {}

        self.friendly_name = self.args['friendly_name']

        self.switch_state = self.args['switch_state']
        self.sensor_power = self.args['sensor_power']
        self.delay_on = self.args['delay_on']
        self.delay_off = self.args['delay_off']
        self.threshold = self.args['threshold']
        self.notifiers = self.args.get('notifiers', [])

        self.timer_handle_list[TIMER_ON] = None
        self.timer_handle_list[TIMER_OFF] = None

        self.device_on = False

        # Subscribe to sensors
        self.listen_state_handle_list[self.sensor_power] = self.listen_state(
            self.state_change, self.sensor_power)

    def state_change(self, entity, attribute, old, new, kwargs):
        if self.get_state(self.switch_state) == 'on':
            if new != None and new != "":
                self.log("Power Usage is: {}".format(float(new)), level="DEBUG")

                # Device not started yet
                if not self.device_on:
                    # Power usage goes up
                    if self.timer_handle_list[TIMER_ON] == None and float(new) > self.threshold:
                        self.log("Possible on detected; starting on timer ({})".format(
                            self.delay_on), level="DEBUG")

                        self.timer_handle_list[TIMER_ON] = self.run_in(
                            self.device_on_timeout, self.delay_on)

                    # Power usage goes down before timeout
                    elif self.timer_handle_list[TIMER_ON] != None and float(new) <= self.threshold:
                        self.log("Cancelling on timer", level="DEBUG")

                        self.cancel_timer(self.timer_handle_list[TIMER_ON])
                        self.timer_handle_list[TIMER_ON] = None

                # Device running
                else:
                    # Power usage goes down below threshold
                    if self.timer_handle_list[TIMER_OFF] == None and float(new) <= self.threshold:
                        self.log("Possible off detected; starting off timer ({})".format(
                            self.delay_off), level="DEBUG")

                        self.timer_handle_list[TIMER_OFF] = self.run_in(
                            self.device_off_timeout, self.delay_off)

                    # Power usage goes up before delay
                    elif self.timer_handle_list[TIMER_OFF] != None and float(new) > self.threshold:
                        self.log("Cancelling off timer")

                        self.cancel_timer(self.timer_handle_list[TIMER_OFF])
                        self.timer_handle_list[TIMER_OFF] = None

    def device_on_timeout(self, kwargs):
        self.device_on = True
        self.timer_handle_list[TIMER_ON] = None

        self._notify("{} started".format(self.friendly_name))

    def device_off_timeout(self, kwargs):
        """Notify User that device is off. This may get cancelled if it turns on again in the meantime"""
        self.device_on = False
        self.timer_handle_list[TIMER_OFF] = None

        self._notify("{} ready".format(self.friendly_name))

    def _notify(self, message):
        self.log("Sending notification: {}".format(message), level="INFO")
        for notifier in self.notifiers:
            self.notify(message, name=notifier)

    def terminate(self):
        for handle in self.timer_handle_list.values():
            if handle != None:
                self.cancel_timer(handle)

        for handle in self.listen_event_handle_list.values():
            if handle != None:
                self.cancel_listen_event(handle)

        for handle in self.listen_state_handle_list.values():
            if handle != None:
                self.cancel_listen_state(handle)
