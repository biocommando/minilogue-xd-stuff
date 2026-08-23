function getFullSequence(steps, offset = 0, centering = false) {
    const midiCc = []
    function sendSeqCc(value) {
        //midiDev.cc(seqControllerNumber, value, midiDevChannel)
        midiCc.push(value)
    }

    // Reset sequence
    midiCc.push(0, 127, 0, 127, 0, 127)

    steps.forEach(step => {
        if (step.type === 'off')
            midiCc.push(1)
        if (centering) {
            centering = false
            offset += 64 - step.note
        }
        midiCc.push(step.note + offset)
        let deltaWords = 1
        if (step.delta !== 0) {
            const deltaBits = Math.log2(step.delta)
            deltaWords = Math.ceil(deltaBits / 5)
        }
        for (let wi = deltaWords - 1; wi >= 0; wi--) {
            let word = (((step.delta >> (wi * 5)) & 0x1f) << 1) | 1;
            if (midiCc[midiCc.length - 1] == word)
                midiCc.push(126) // Don't care -> prevents synth from gating consequent same values
            midiCc.push(word)
        }
        // Note separator
        midiCc.push(0)
    })

    // Lock sequence
    midiCc.push(127, 0)
    return midiCc
}

/*console.log(getFullSequence([
{"note": 72, "delta": 88090},
{"note": 1, "delta": 230},
{"note": 79, "delta": 87400},
{"note": 1, "delta": 920},
{"note": 82, "delta": 29210},
{"note": 1, "delta": 3910},
{"note": 84, "delta": 32890},
{"note": 1, "delta": 230},
{"note": 82, "delta": 20010},
{"note": 1, "delta": 2070},
{"note": 79, "delta": 86020},
{"note": 1, "delta": 2300},
{"note": 75, "delta": 88090},
{"note": 1, "delta": 230},
{"note": 77, "delta": 87860},
{"note": 1, "delta": 460},
{"note": 74, "delta": 86250},
{"note": 1, "delta": 2070},
{"note": 70, "delta": 10350},
{"note": 1, "delta": 690},
{"note": 68, "delta": 21620},
{"note": 1, "delta": 460},
{"note": 67, "delta": 43930},
{"note": 1, "delta": 11270}]).join('\n'))*/
