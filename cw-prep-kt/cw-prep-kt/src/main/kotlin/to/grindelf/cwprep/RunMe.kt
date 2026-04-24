package to.grindelf.cwprep

import java.security.MessageDigest
import kotlin.math.log2

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
}

class Tree {
    val root = Node('/')

    fun addPath(path: String) {
        var current = root
        for (ch in path.drop(1)) {
            if (ch == '/') continue
            current = current.children.getOrPut(ch) { Node(ch) }
        }
    }

    private fun computeHashes(node: Node, hashCount: MutableMap<String, Int>): String {
        val childHashes = node.children.values.map { computeHashes(it, hashCount) }
            .sorted()

        val canonical = "${node.name}(${childHashes.joinToString(",")})"
        val hash = sha256(canonical)

        hashCount[hash] = (hashCount[hash] ?: 0) + 1
        return hash
    }

    fun countDuplicateSubtrees(): Int {
        val hashCount = mutableMapOf<String, Int>()
        for (child in root.children.values) {
            computeHashes(child, hashCount)
        }
        return hashCount.values.count { it >= 2 }
    }

    fun minMaxDistance(nameA: Char, nameB: Char): String {
        // Step 1: flatten the tree — assign each node an ID and record depth + parent
        val log = (log2(10_000.0) + 2).toInt()

        val allNodes  = mutableListOf<Node>()
        val depth     = mutableMapOf<Node, Int>()
        val up        = mutableMapOf<Node, Array<Node?>>()

        // DFS (iterative to avoid stack overflow on deep trees)
        val stack = ArrayDeque<Pair<Node, Node?>>()  // (current, parent)
        stack.addLast(root to null)
        while (stack.isNotEmpty()) {
            val (node, parent) = stack.removeLast()
            allNodes.add(node)
            val d = if (parent == null) 0 else depth[parent]!! + 1
            depth[node] = d
            val anc = arrayOfNulls<Node>(log)
            anc[0] = parent ?: root          // root's parent = itself (sentinel)
            up[node] = anc
            for (child in node.children.values) stack.addLast(child to node)
        }

        for (k in 1 until log) {
            for (node in allNodes) {
                val anc = up[node]!!
                anc[k] = up[anc[k - 1]!!]!![k - 1]
            }
        }

        fun lca(a: Node, b: Node): Node {
            var u = a; var v = b
            var diff = depth[u]!! - depth[v]!!
            if (diff < 0) { val tmp = u; u = v; v = tmp; diff = -diff }
            for (k in 0 until log) if (diff shr k and 1 == 1) u = up[u]!![k]!!
            if (u === v) return u
            for (k in log - 1 downTo 0)
                if (up[u]!![k] !== up[v]!![k]) { u = up[u]!![k]!!; v = up[v]!![k]!! }
            return up[u]!![0]!!
        }

        fun dist(a: Node, b: Node): Int {
            val l = lca(a, b)
            return depth[a]!! + depth[b]!! - 2 * depth[l]!!
        }

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

    private fun sha256(input: String): String {
        val digest = MessageDigest.getInstance("SHA-256")
        val bytes = digest.digest(input.toByteArray(Charsets.UTF_8))
        return bytes.joinToString("") { "%02x".format(it) }
    }
}
