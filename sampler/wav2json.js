// JavaScript port of wav2json.c: converts a wav file into the shift-shape
// CC value sequence understood by the sampler oscillator (see README.md).

const SEG_WAVE = 1
const SEG_SAMPLERATE = 2
const SEG_BASEFREQ = 3
const SEG_LOOPIDX = 4
const SEG_BITS = 5

const DATA_LEN = 30000

function noteToFrequency(note) {
    if (note.length < 2)
        return 0
    const noteIdxs = { C: 0, D: 2, E: 4, F: 5, G: 7, A: 9, B: 11 }
    let noteIdx = noteIdxs[note[0]] || 0
    let oct = note.slice(1)
    if (note[1] === '#' || note[1] === 'b') {
        if (note[1] === '#')
            noteIdx++
        else
            noteIdx--
        oct = note.slice(2)
    }
    let octave = parseInt(oct, 10)
    if (isNaN(octave))
        octave = 4
    noteIdx += (octave + 1) * 12
    return 440 * Math.pow(2, (noteIdx - 69.0) / 12.0)
}

// Parses a RIFF wav file. Returns an object with sampleRate, channels,
// numFrames and get(frame) that returns the first channel's value
// normalized like wav_get_normalized() does.
function parseWav(buffer) {
    const view = new DataView(buffer)
    const tag = pos => String.fromCharCode(...new Uint8Array(buffer, pos, 4))
    if (buffer.byteLength < 12 || tag(0) !== 'RIFF' || tag(8) !== 'WAVE')
        throw 'not a RIFF WAVE file'
    let fmt = null, dataPos = -1, dataLen = 0
    for (let pos = 12; pos + 8 <= buffer.byteLength;) {
        const id = tag(pos), len = view.getUint32(pos + 4, true)
        if (id === 'fmt ') {
            let fmtType = view.getUint16(pos + 8, true)
            // WAVE_FORMAT_EXTENSIBLE: actual format is in the sub format GUID
            if (fmtType === 0xFFFE && len >= 40)
                fmtType = view.getUint16(pos + 32, true)
            fmt = {
                fmtType,
                channels: view.getUint16(pos + 10, true),
                sampleRate: view.getUint32(pos + 12, true),
                bitDepth: view.getUint16(pos + 22, true),
            }
        } else if (id === 'data') {
            dataPos = pos + 8
            dataLen = Math.min(len, buffer.byteLength - dataPos)
            break
        }
        pos += 8 + len + (len & 1)
    }
    if (!fmt)
        throw 'fmt chunk not found'
    if (dataPos < 0)
        throw 'data chunk not found'
    const { fmtType, channels, sampleRate, bitDepth } = fmt
    const isFloat = fmtType === 3
    if (fmtType !== 1 && !isFloat)
        throw 'unsupported wav format ' + fmtType
    if (![8, 16, 24, 32, 64].includes(bitDepth) || (bitDepth === 64 && !isFloat))
        throw 'unsupported bit depth ' + bitDepth
    const byteDepth = bitDepth / 8
    const numFrames = Math.floor(dataLen / (channels * byteDepth))
    // Math.fround() mimics the single precision float math of the C version
    const f = Math.fround
    const get = frame => {
        const p = dataPos + frame * channels * byteDepth
        switch (bitDepth) {
            case 8: return f(f(f(view.getUint8(p)) / 0xFF) * 2 - 1)
            case 16: return f(view.getInt16(p, true) / (0x8000 - 1))
            case 24: return f(((view.getUint8(p + 2) << 24 | view.getUint16(p, true) << 8) >> 8) / (0x800000 - 1))
            case 32: return isFloat ? view.getFloat32(p, true) : f(f(view.getInt32(p, true)) / (0x80000000 - 1))
            case 64: return f(view.getFloat64(p, true))
        }
    }
    return { sampleRate, channels, bitDepth, numFrames, get }
}

// Options: bits (8 or 16), freq (base frequency in Hz, 0 = not sent),
// loopIdx (loop index). Returns { seq, info }.
function wavToSequence(buffer, { bits = 8, freq = 0, loopIdx = DATA_LEN } = {}) {
    const wav = parseWav(buffer)
    const seq = []
    let seqNum = 0
    const word = (w, nBytes) => {
        for (let shift = (nBytes * 2 - 1) * 4; shift >= 0; shift -= 4) {
            seq.push((seqNum << 4) | ((w >>> shift) & 0xF))
            seqNum = (seqNum + 1) % 4
        }
    }
    const segStart = segment => {
        seq.push(0x40 | segment)
        seqNum = 0
    }
    // META=7,6 to be extra sure that reset goes through
    const reset = () => seq.push(112, 96)

    reset()
    segStart(SEG_WAVE)
    const maxFrames = Math.min(wav.numFrames, bits === 16 ? DATA_LEN / 2 : DATA_LEN)
    let maxval = 0
    for (let i = 0; i < maxFrames; i++)
        maxval = Math.max(maxval, Math.abs(wav.get(i)))
    if (maxval === 0)
        maxval = 1
    const range = bits === 16 ? 0x7fff : 127
    for (let i = 0; i < maxFrames; i++) {
        let w = Math.trunc(Math.fround(Math.fround(wav.get(i) / maxval) * range))
        if (w < 0)
            w = range - w
        word(w, bits === 16 ? 2 : 1)
    }
    reset()

    if (wav.sampleRate !== 16000) {
        segStart(SEG_SAMPLERATE)
        word(wav.sampleRate, 2)
        reset()
    }

    if (freq) {
        segStart(SEG_BASEFREQ)
        word(Math.trunc(Math.fround(freq) * 1e6) >>> 0, 4)
        reset()
    }

    segStart(SEG_LOOPIDX)
    word(loopIdx, 2)
    reset()

    if (bits !== 8) {
        segStart(SEG_BITS)
        word(bits, 1)
        reset()
    }

    return {
        seq,
        info: {
            sampleRate: wav.sampleRate,
            channels: wav.channels,
            bitDepth: wav.bitDepth,
            numFrames: wav.numFrames,
            usedFrames: maxFrames,
        },
    }
}

if (typeof module !== 'undefined')
    module.exports = { noteToFrequency, parseWav, wavToSequence }
