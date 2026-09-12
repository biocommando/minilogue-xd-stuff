class MidiDevice {
    constructor(type, device) {
        this.device = device
        this.onNoteOn = () => 0
        this.onNoteOff = () => 0
        if (type === 'input') {
            device.onmidimessage = msg => this.onMidiMessage(msg.data)
        }
    }

    onMidiMessage(data) {
        if ((data[0] & 0xF0) === 0b10010000) {
            this.onNoteOn(data[0] & 0x0F, data[1], data[2])
        }
        else if ((data[0] & 0xF0) === 0b10000000) {
            this.onNoteOff(data[0] & 0x0F, data[1])
        }
    }

    midiCmd(cmd) {
        this.device.send(cmd)
    }

    noteOn(note, channel, velocity) {
        const midiCmd = 0b10010000;
        this.midiCmd([midiCmd | channel, note, velocity !== undefined ? velocity : 127]);
    }

    noteOff(note, channel) {
        const midiCmd = 0b10000000;
        this.midiCmd([midiCmd | channel, note, 0]);
    }

    cc(controllerNumber, value, channel) {
        const midiCmd = 0b10110000;
        this.midiCmd([midiCmd | channel, controllerNumber, value]);
    }
}

class MidiDeviceList {
    async init() {
        const midiAccess = await navigator.requestMIDIAccess({ sysex: true })
        this.inputs = []
        midiAccess.inputs.forEach(x => this.inputs.push(x))
        this.outputs = []
        midiAccess.outputs.forEach(x => this.outputs.push(x))
    }

    getInputs() {
        return this.inputs.map((x, i) => ({name: x.name, id: i}))
    }

    getOutputs() {
        return this.outputs.map((x, i) => ({name: x.name, id: i}))
    }

    getDevice(type, id) {
        if (!['input', 'output'].includes(type)) {
            throw 'invalid device type'
        }
        const list = type === 'input' ? this.inputs : this.outputs
        const device = list[Number(id)]
        if (device) {
            device.open()
            return new MidiDevice(type, device)
        }
    }
}
