/*
*
*  Author: Shipunov Greg
*  Variant 1
*
*/
package to.grindelf.cw

object RunMe {
    @JvmStatic
    fun main(args: Array<String>) {
        val mdfm = MinimalDeterminedFiniteMachine()
        val lines = mutableListOf<String>()
        var line = readlnOrNull()
        while (!line.isNullOrBlank()) {
            lines.add(line.trim())
            line = readlnOrNull()
        }
        for (sequence in lines) mdfm.addVertex(sequence)

        println(mdfm)
    }
}

data class State(
    val id: Int,
    val layer: Int
) {
    val transitions: MutableMap<Char, State> = mutableMapOf()

    fun fin() = transitions.isEmpty()
}

class MinimalDeterminedFiniteMachine {
    val root: State = State(id = 0, layer = 0)
    var fin: State = root
    var nextId: Int = 1
    var maxLayer: Int = 0
    var notFirstRun = false
    val layers: MutableMap<Int, MutableList<State>> = mutableMapOf()

    fun addVertex(sequence: String) {
        if (maxLayer == 0) maxLayer = sequence.length
        var currentLayer: Int = 1
        var current: State = root

        for (i in 0 until sequence.length) {
            val c = sequence[i]
            val c1: Char = if (i + 1 < sequence.length) sequence[i + 1] else '-'

            if (current.transitions.contains(c)) {
                current = current.transitions[c]!!
                currentLayer++
                continue
            }

            if (notFirstRun && c1 != '-') {
                var fittingSkip: State = State(-1, -1)
                loop@ for ((ch, state) in current.transitions) {
                    if (state.transitions.keys.size == 1 && state.transitions.keys.first() == c1) {
                        fittingSkip = state
                        break@loop
                    }
                }
                if (fittingSkip != State(-1, -1)) {
                    current.transitions[c] = fittingSkip
                    current = current.transitions[c]!!
                    continue
                }
            }

            current.transitions[c] = State(id = nextId++, layer = currentLayer)
            if (layers[currentLayer] == null) layers[currentLayer] = mutableListOf()
            else layers[currentLayer]!!.add(current)
            currentLayer++
            current = current.transitions[c]!!
            if (fin.layer != maxLayer) fin = current

            if (current.layer == maxLayer - 1 && fin.layer == maxLayer) {
                current.transitions[c] = fin
                break
            }

        }
        notFirstRun = true
    }

    fun minimizeMachine() {

    }


}

