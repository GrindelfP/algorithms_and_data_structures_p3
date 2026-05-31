import java.util.concurrent.*
import java.util.concurrent.atomic.AtomicInteger
import kotlin.math.min
import kotlin.random.Random

typealias Bigint = ULong

// ──────────────────────────────────────────────
// Вычислительно тяжёлая часть
// ──────────────────────────────────────────────

fun fibonacci(n: Bigint): Bigint =
    if (n < 2UL) n else fibonacci(n - 1UL) + fibonacci(n - 2UL)

fun special(n: Int): Boolean {
    var sum = 0UL
    for (i in 0 until n) sum += fibonacci(i.toULong())
    return sum % 2UL == 0UL
}

// ──────────────────────────────────────────────
// 1. Однопоточная реализация
// ──────────────────────────────────────────────

fun single(v: List<Int>): Long =
    v.count { special(it) }.toLong()

// ──────────────────────────────────────────────
// 2. Блочная реализация (block)
//    Каждый поток получает фиксированный диапазон.
//    При отсортированном массиве первые блоки тяжелее —
//    нагрузка неравномерна, ускорение хуже.
// ──────────────────────────────────────────────

fun block(v: List<Int>, nThreads: Int): Long {
    val results = LongArray(nThreads)
    val partSize = v.size / nThreads
    val threads = Array(nThreads) { t ->
        val a = t * partSize
        val b = if (t == nThreads - 1) v.size else a + partSize
        Thread {
            results[t] = v.subList(a, b).count { special(it) }.toLong()
        }
    }
    threads.forEach { it.start() }
    threads.forEach { it.join() }
    return results.sum()
}

// ──────────────────────────────────────────────
// 3. Очередь задач через LinkedBlockingQueue  (queue)
//    Потоки динамически берут задания из очереди —
//    нагрузка распределяется равномерно независимо
//    от порядка элементов.
// ──────────────────────────────────────────────

fun queue(v: List<Int>, nThreads: Int): Long {
    val taskQueue = LinkedBlockingQueue<Int>(v.size + nThreads)
    v.forEach { taskQueue.put(it) }
    // Sentinel-значение: сигнал завершения для каждого потока
    repeat(nThreads) { taskQueue.put(Int.MIN_VALUE) }

    val counter = AtomicInteger(0)

    val threads = Array(nThreads) {
        Thread {
            while (true) {
                val task = taskQueue.take()
                if (task == Int.MIN_VALUE) break   // сигнал «конец»
                if (special(task)) counter.incrementAndGet()
            }
        }
    }

    threads.forEach { it.start() }
    threads.forEach { it.join() }
    return counter.get().toLong()
}

// ──────────────────────────────────────────────
// 4. Очередь задач через ThreadPoolExecutor  (pool)
//    Аналог queue, но с пулом потоков JDK.
//    Демонстрирует другую технологию синхронизации.
// ──────────────────────────────────────────────

fun threadpool(v: List<Int>, nThreads: Int): Long {
    val counter = AtomicInteger(0)
    val latch = CountDownLatch(v.size)
    val executor = ThreadPoolExecutor(
        nThreads, nThreads,
        0L, TimeUnit.MILLISECONDS,
        LinkedBlockingQueue()
    )

    v.forEach { elem ->
        executor.submit {
            if (special(elem)) counter.incrementAndGet()
            latch.countDown()
        }
    }

    latch.await()
    executor.shutdown()
    return counter.get().toLong()
}

// ──────────────────────────────────────────────
// Вспомогательные функции
// ──────────────────────────────────────────────

fun timeMs(block: () -> Long): Pair<Long, Long> {
    val t1 = System.currentTimeMillis()
    val result = block()
    val t2 = System.currentTimeMillis()
    return Pair(t2 - t1, result)
}

// ──────────────────────────────────────────────
// main
// ──────────────────────────────────────────────

fun main() {
    // Генерация массива — повторяем логику C++ теста:
    // Poisson(4) + 40, cap 53, seed=1
    val rng = Random(1)
    val n = 50
    val v = List(n) {
        val raw = 40 + generatePoisson(4.0, rng)
        min(raw, 53)
    }.sortedDescending()

    println("# Массив (отсортированный): ${v.joinToString(",")}")
    println()
    println("%-10s %-12s %-12s %-12s %-12s %-10s".format(
        "nthreads", "single(ms)", "block(ms)", "queue(ms)", "pool(ms)", "result"
    ))
    println("-".repeat(72))

    // Однопоточное выполнение
    val (singleTime, singleResult) = timeMs { single(v) }
    println("%-10d %-12d %-12s %-12s %-12s %-10d".format(
        1, singleTime, singleTime, singleTime, singleTime, singleResult
    ))

    // Данные для итогового CSV
    val csvLines = mutableListOf<String>()
    csvLines.add("nthreads,single_ms,block_ms,queue_ms,pool_ms,block_result,queue_result,pool_result")
    csvLines.add("1,$singleTime,$singleTime,$singleTime,$singleTime,$singleResult,$singleResult,$singleResult")

    for (th in listOf(2, 3, 4, 5)) {
        val (blockTime, blockResult) = timeMs { block(v, th) }
        val (queueTime, queueResult) = timeMs { queue(v, th) }
        val (poolTime, poolResult)   = timeMs { threadpool(v, th) }

        println("%-10d %-12d %-12d %-12d %-12d %-10s".format(
            th, singleTime, blockTime, queueTime, poolTime,
            "$blockResult/$queueResult/$poolResult"
        ))

        csvLines.add("$th,$singleTime,$blockTime,$queueTime,$poolTime,$blockResult,$queueResult,$poolResult")
    }

    // Запись CSV для Python-скрипта
    java.io.File("results.csv").writeText(csvLines.joinToString("\n"))
    println()
    println("Результаты сохранены в results.csv")
}

// Генерация числа Пуассона вручную (метод Кнута)
fun generatePoisson(lambda: Double, rng: Random): Int {
    val l = Math.exp(-lambda)
    var k = 0
    var p = 1.0
    do {
        k++
        p *= rng.nextDouble()
    } while (p > l)
    return k - 1
}

