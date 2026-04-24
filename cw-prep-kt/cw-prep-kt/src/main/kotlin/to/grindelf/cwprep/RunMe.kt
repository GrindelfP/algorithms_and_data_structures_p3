package to.grindelf.cwprep

import java.security.MessageDigest
import kotlin.math.log2

private val LOG = (log2(10_000.0) + 2).toInt()

fun main() {
    task01()
    task02()
}

fun task01() {
    val tree = Tree()
    val lines = mutableListOf<String>()
    var line = readlnOrNull()
    while (!line.isNullOrBlank()) {
        lines.add(line.trim())
        line = readlnOrNull()
    }

    val query = lines.removeLast().split(" ")
    val nameA = query[0][0]
    val nameB = query[1][0]

    for (path in lines) tree.addPath(path)

    tree.prepareForLca()
    println(tree.minMaxDistance(nameA, nameB))
}

fun task02() {
    val tree = Tree()
    var line = readlnOrNull()
    while (!line.isNullOrBlank()) {
        tree.addPath(line.trim())
        line = readlnOrNull()
    }
    println(tree.countDuplicateSubtrees())
}

class Node(val name: Char) {
    val children: MutableMap<Char, Node> = mutableMapOf()
    var depth: Int = 0
    val up: Array<Node?> = arrayOfNulls(LOG)
}

class Tree {
    val root = Node('/')
    private val allNodes = mutableListOf<Node>()

    fun addPath(path: String) {
        var current = root
        for (ch in path.drop(1)) {
            if (ch == '/') continue
            current = current.children.getOrPut(ch) {
                Node(ch).also { it.depth = current.depth + 1 }
            }
        }
    }

    fun prepareForLca() {
        allNodes.clear()

        val stack = ArrayDeque<Pair<Node, Node?>>()
        stack.addLast(root to null)

        while (stack.isNotEmpty()) {
            val (node, parent) = stack.removeLast()
            allNodes.add(node)
            node.depth = if (parent == null) 0 else parent.depth + 1
            node.up[0] = parent ?: root
            for (child in node.children.values) stack.addLast(child to node)
        }

        for (k in 1 until LOG) {
            for (node in allNodes) {
                node.up[k] = node.up[k - 1]!!.up[k - 1]
            }
        }
    }

    private fun lca(a: Node, b: Node): Node {
        var u = a
        var v = b

        var diff = u.depth - v.depth
        if (diff < 0) { val tmp = u; u = v; v = tmp; diff = -diff }

        for (k in 0 until LOG) {
            if (diff shr k and 1 == 1) u = u.up[k]!!
        }

        if (u === v) return u

        for (k in LOG - 1 downTo 0) {
            if (u.up[k] !== v.up[k]) {
                u = u.up[k]!!
                v = v.up[k]!!
            }
        }

        return u.up[0]!!
    }

    private fun dist(a: Node, b: Node): Int {
        val l = lca(a, b)
        return a.depth + b.depth - 2 * l.depth
    }

    fun minMaxDistance(nameA: Char, nameB: Char): String {
        val nodesA = allNodes.filter { it.name == nameA }
        val nodesB = allNodes.filter { it.name == nameB }

        var minD = Int.MAX_VALUE
        var maxD = Int.MIN_VALUE

        for (a in nodesA) for (b in nodesB) {
            val d = dist(a, b)
            if (d < minD) minD = d
            if (d > maxD) maxD = d
        }

        return "$minD $maxD"
    }

    fun countDuplicateSubtrees(): Int {
        val hashCount = mutableMapOf<String, Int>()
        for (child in root.children.values) {
            computeHashes(child, hashCount)
        }
        return hashCount.values.count { it >= 2 }
    }

    private fun computeHashes(node: Node, hashCount: MutableMap<String, Int>): String {
        val childHashes = node.children.values.map { computeHashes(it, hashCount) }.sorted()
        val canonical = "${node.name}(${childHashes.joinToString(",")})"
        val hash = sha256(canonical)
        hashCount[hash] = (hashCount[hash] ?: 0) + 1
        return hash
    }

    private fun sha256(input: String): String {
        val digest = MessageDigest.getInstance("SHA-256")
        val bytes = digest.digest(input.toByteArray(Charsets.UTF_8))
        return bytes.joinToString("") { "%02x".format(it) }
    }
}
