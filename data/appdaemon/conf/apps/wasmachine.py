import appdaemon.plugins.hass.hassapi as hass


class WasmachineMonitor(hass.Hass):
    def initialize(self):
        self.timer_handle_list = []
        self.listen_event_handle_list = []
        self.listen_state_handle_list = []

        self.switch_state = self.args['switch_state']
        self.sensor_power = self.args['sensor_power']
        self.delay = self.args['delay']
        self.threshold = self.args['threshold']
        self.notifiers = self.args.get('notifiers', [])

        self.triggered = False
        self.isWaitingHandle = None

        # Subscribe to sensors
        self.listen_state_handle_list.append(
            self.listen_state(self.state_change, self.sensor_power))

    def state_change(self, entity, attribute, old, new, kwargs):
        if self.get_state(self.switch_state) == 'on':
            # Initial: power usage goes up
            if (new != None and new != "" and not self.triggered and float(new) > self.threshold):
                self.triggered = True
                self.log("Power Usage is: {}".format(float(new)))

                self.notify_device_on()
            # Power usage goes down below threshold
            elif (new != None and new != "" and self.triggered and self.isWaitingHandle == None and float(new) <= self.threshold):
                self.log("Waiting: {} seconds to notify.".format(self.delay))
                self.isWaitingHandle = self.run_in(
                    self.notify_device_off, self.delay)

                self.timer_handle_list.append(self.isWaitingHandle)
            # Power usage goes up before delay
            elif(new != None and new != "" and self.triggered and self.isWaitingHandle != None and float(new) > self.threshold):
                self.log("Cancelling timer")
                self.cancel_timer(self.isWaitingHandle)
                self.isWaitingHandle = None

    def notify_device_on(self):
        self._notify("Wasmachine started")

    def notify_device_off(self, kwargs):
        """Notify User that device is off. This may get cancelled if it turns on again in the meantime"""
        self.triggered = False
        self.log("Setting triggered to: {}".format(self.triggered))
        self.isWaitingHandle = None
        self.log("Setting isWaitingHandle to: {}".format(self.isWaitingHandle))
        self.log("Notifying user")

        self._notify("Wasmachine ready")

    def _notify(self, message):
        for notifier in self.notifiers:
            self.notify(message, name=notifier)

    def terminate(self):
        for timer_handle in self.timer_handle_list:
            self.cancel_timer(timer_handle)

        for listen_event_handle in self.listen_event_handle_list:
            self.cancel_listen_event(listen_event_handle)

        for listen_state_handle in self.listen_state_handle_list:
            self.cancel_listen_state(listen_state_handle)
