use std::time::Instant;
use std::fs;
use std::io::Write;

// ─── Naive ───────────────────────────────────────────────────────────────────

fn naive(text: &[u8], pattern: &[u8]) -> Option<usize> {
    let n = text.len();
    let m = pattern.len();
    if m == 0 { return Some(0); }
    if m > n  { return None; }
    'outer: for i in 0..=(n - m) {
        for j in 0..m {
            if text[i + j] != pattern[j] { continue 'outer; }
        }
        return Some(i);
    }
    None
}

// ─── Knuth-Morris-Pratt (оптимизированная таблица kmpNext) ───────────────────


fn build_kmp_next(pattern: &[u8]) -> Vec<i64> {
    let m = pattern.len();
    let mut next = vec![0i64; m + 1];
    next[0] = -1;
    let mut k: i64 = -1;
    for i in 0..m {
        while k >= 0 && pattern[i] != pattern[k as usize] {
            k = next[k as usize];
        }
        k += 1;
        if i + 1 < m && pattern[i + 1] == pattern[k as usize] {
            next[i + 1] = next[k as usize];
        } else {
            next[i + 1] = k;
        }
    }
    next
}

fn kmp(text: &[u8], pattern: &[u8], next: &[i64]) -> Option<usize> {
    let n = text.len();
    let m = pattern.len();
    if m == 0 { return Some(0); }
    if m > n  { return None; }
    let mut j: i64 = 0;
    for i in 0..n {
        while j >= 0 && text[i] != pattern[j as usize] {
            j = next[j as usize];
        }
        j += 1;
        if j == m as i64 {
            return Some(i + 1 - m);
        }
    }
    None
}

// ─── Boyer-Moore (bad character + good suffix) ────────────────────────────────

const ALPHABET: usize = 256;

fn bad_char_table(pattern: &[u8]) -> [i64; ALPHABET] {
    let mut bc = [-1i64; ALPHABET];
    for (i, &c) in pattern.iter().enumerate() {
        bc[c as usize] = i as i64;
    }
    bc
}

fn good_suffix_table(pattern: &[u8]) -> Vec<usize> {
    let m = pattern.len();
    let mut gs   = vec![m; m + 1];
    let mut bpos = vec![0usize; m + 1];
    let mut i = m as i64;
    let mut j = m as i64 + 1;
    bpos[i as usize] = j as usize;
    while i > 0 {
        while j <= m as i64 && pattern[(i - 1) as usize] != pattern[(j - 1) as usize] {
            if gs[j as usize] == m { gs[j as usize] = (j - i) as usize; }
            j = bpos[j as usize] as i64;
        }
        i -= 1; j -= 1;
        bpos[i as usize] = j as usize;
    }
    j = bpos[0] as i64;
    for i in 0..=m as i64 {
        if gs[i as usize] == m { gs[i as usize] = j as usize; }
        if i == j { j = bpos[j as usize] as i64; }
    }
    gs
}

fn bm(text: &[u8], pattern: &[u8]) -> Option<usize> {
    let n = text.len();
    let m = pattern.len();
    if m == 0 { return Some(0); }
    if m > n  { return None; }
    let bc = bad_char_table(pattern);
    let gs = good_suffix_table(pattern);
    let mut s: i64 = 0;
    while s <= (n as i64 - m as i64) {
        let mut j = m as i64 - 1;
        while j >= 0 && pattern[j as usize] == text[(s + j) as usize] { j -= 1; }
        if j < 0 { return Some(s as usize); }
        let bc_shift = j - bc[text[(s + j) as usize] as usize];
        let gs_shift = gs[(j + 1) as usize] as i64;
        s += bc_shift.max(gs_shift);
    }
    None
}

// ─── Benchmark ───────────────────────────────────────────────────────────────

struct BenchResult {
    name:     String,
    times_us: Vec<u128>,
    found:    Option<usize>,
}

fn bench<F>(name: &str, warmup: usize, runs: usize, f: F) -> BenchResult
where
    F: Fn() -> Option<usize>,
{
    let mut found = None;
    for _ in 0..warmup { found = f(); }

    let mut times = Vec::with_capacity(runs);
    for _ in 0..runs {
        let t0 = Instant::now();
        found = f();
        times.push(t0.elapsed().as_micros());
    }
    BenchResult { name: name.to_string(), times_us: times, found }
}

fn stats(times: &[u128]) -> (f64, u128, u128, f64) {
    let mean = times.iter().sum::<u128>() as f64 / times.len() as f64;
    let min  = *times.iter().min().unwrap();
    let max  = *times.iter().max().unwrap();
    let mut sorted = times.to_vec();
    sorted.sort_unstable();
    let median = if sorted.len() % 2 == 0 {
        (sorted[sorted.len() / 2 - 1] + sorted[sorted.len() / 2]) as f64 / 2.0
    } else {
        sorted[sorted.len() / 2] as f64
    };
    (mean, min, max, median)
}

// ─── Main ─────────────────────────────────────────────────────────────────────

fn main() {
    let text_bytes: Vec<u8> = match fs::read("simplewiki-20260201.txt") {
        Ok(data) => {
            eprintln!("Загружено {} байт из simplewiki-20260201.txt", data.len());
            data
        }
        Err(_) => {
            eprintln!("Файл не найден — генерирую ~50 МБ синтетического текста.");
            generate_synthetic_text()
        }
    };

    // Оригинальный паттерн из задания
    let pattern: &[u8] = b"and is the second single from their greatest hits";

    let kmp_next = build_kmp_next(pattern);

    const WARMUP: usize = 5;
    const RUNS:   usize = 35;

    println!("Длина текста : {} байт", text_bytes.len());
    println!("Подстрока    : {:?}", std::str::from_utf8(pattern).unwrap());
    println!("Прогрев      : {} запусков", WARMUP);
    println!("Замеры       : {} запусков\n", RUNS);
    println!("{}", "=".repeat(55));

    let naive_result = bench("Наивный",   WARMUP, RUNS, || naive(&text_bytes, pattern));
    let kmp_result   = bench("КМП",       WARMUP, RUNS, || kmp(&text_bytes, pattern, &kmp_next));
    let bm_result    = bench("Бойер-Мур", WARMUP, RUNS, || bm(&text_bytes, pattern));

    for res in [&naive_result, &kmp_result, &bm_result] {
        let (mean, min, max, median) = stats(&res.times_us);
        println!("{}", res.name);
        println!("  Найдено на позиции : {:?}", res.found);
        println!("  Среднее время      : {:.1} мкс", mean);
        println!("  Медиана            : {:.1} мкс", median);
        println!("  Мин / Макс         : {} / {} мкс", min, max);
        println!("  Все замеры (мкс)   : {:?}\n", res.times_us);
    }

    println!("{}", "=".repeat(55));
    println!("СВОДНАЯ ТАБЛИЦА (среднее, мкс)");
    println!("{}", "-".repeat(35));
    for res in [&naive_result, &kmp_result, &bm_result] {
        let mean = res.times_us.iter().sum::<u128>() as f64 / res.times_us.len() as f64;
        println!("  {:<28} {:>8.1}", res.name, mean);
    }
    println!("{}", "-".repeat(35));

    // CSV 1: средние значения
    {
        let avg = |r: &BenchResult| -> f64 {
            r.times_us.iter().sum::<u128>() as f64 / r.times_us.len() as f64
        };
        let mut f = fs::File::create("average_results.csv")
            .expect("Не удалось создать average_results.csv");
        writeln!(f, "naive_average,kmp_average,bm_average").unwrap();
        writeln!(f, "{:.1},{:.1},{:.1}",
                 avg(&naive_result), avg(&kmp_result), avg(&bm_result)).unwrap();
        println!("\nCSV сохранён: average_results.csv");
    }

    // CSV 2: все замеры по запускам
    {
        let mut f = fs::File::create("benchmark_results.csv")
            .expect("Не удалось создать benchmark_results.csv");
        writeln!(f, "run,naive,kmp,bm").unwrap();
        for i in 0..RUNS {
            writeln!(f, "{},{},{},{}",
                     i + 1,
                     naive_result.times_us[i],
                     kmp_result.times_us[i],
                     bm_result.times_us[i]).unwrap();
        }
        println!("CSV сохранён: benchmark_results.csv");
    }
}

// ─── Synthetic text generator ─────────────────────────────────────────────────

fn generate_synthetic_text() -> Vec<u8> {
    let filler: &[u8] = b"The quick brown fox jumps over the lazy dog near the river bank. \
                          Wikipedia is a free online encyclopedia that anyone can edit freely. \
                          Algorithms are fundamental to computer science and software engineering. \
                          This sentence contains various common English words to simulate real text. ";
    let pattern: &[u8] = b"and is the second single from their greatest hits";

    let target_size = 50 * 1024 * 1024usize;
    let n_chunks    = target_size / filler.len();
    let half        = n_chunks / 2;

    let mut text = Vec::with_capacity(target_size + pattern.len() + 256);
    for _ in 0..half { text.extend_from_slice(filler); }
    text.extend_from_slice(pattern);
    for _ in half..n_chunks { text.extend_from_slice(filler); }
    text
}
