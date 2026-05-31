use std::collections::HashMap;
use std::fs;
use std::time::{Duration, Instant};

// ─────────────────────────────────────────────────────────────────────────────
// Lookup-таблица 256 → u8:
//   0xFF = не буква алфавита
//   0..53 = индекс символа
//
// Строится один раз в compile time через const fn.
// Это убирает branch-дерево match и даёт один load из L1-кэша.
// ─────────────────────────────────────────────────────────────────────────────
const ALPHA_SIZE: usize = 54;
const INVALID: u8 = 0xFF;

const fn build_lut() -> [u8; 256] {
    let mut t = [INVALID; 256];
    let mut c = b'a';
    while c <= b'z' {
        t[c as usize] = c - b'a';
        c += 1;
    }
    let mut c = b'A';
    while c <= b'Z' {
        t[c as usize] = c - b'A' + 26;
        c += 1;
    }
    t[b'\'' as usize] = 52;
    t[b'-' as usize] = 53;
    t
}

static LUT: [u8; 256] = build_lut();

// ─────────────────────────────────────────────────────────────────────────────
// Узел arena-trie.
//
// Размер: 54×4 + 4 = 220 байт.
// children[i] == 0  →  нет потомка (индекс 0 занят корнем, у которого count=0).
// ─────────────────────────────────────────────────────────────────────────────
#[repr(C)]
struct TrieNode {
    children: [u32; ALPHA_SIZE],
    count: u32,
}

impl TrieNode {
    #[inline(always)]
    const fn new() -> Self {
        Self {
            children: [0u32; ALPHA_SIZE],
            count: 0,
        }
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Интерфейс (аналог ITrie)
// ─────────────────────────────────────────────────────────────────────────────
trait ITrie {
    fn insert(&mut self, word: &[u8]);
    fn get(&self, word: &[u8]) -> usize;
    fn nodes(&self) -> usize;
    fn size(&self) -> usize;
}

// ─────────────────────────────────────────────────────────────────────────────
// Trie на arena-аллокаторе
// ─────────────────────────────────────────────────────────────────────────────
struct Trie {
    arena: Vec<TrieNode>,
    unique_words: usize,
}

impl Trie {
    fn with_capacity(cap: usize) -> Self {
        let mut arena = Vec::with_capacity(cap);
        arena.push(TrieNode::new()); // индекс 0 — корень
        Self { arena, unique_words: 0 }
    }
}

impl ITrie for Trie {
    #[inline(always)]
    fn insert(&mut self, word: &[u8]) {
        // SAFETY: arena никогда не пуста (корень всегда есть).
        // get_unchecked убирает bounds-check на каждой итерации —
        // горячий путь в цикле по ~1GB текста.
        let mut cur = 0u32;
        for &b in word {
            let idx = LUT[b as usize];
            if idx == INVALID { return; }
            let idx = idx as usize;

            // Читаем child без bounds-check (cur — всегда валидный индекс,
            // т.к. мы сами его записывали)
            let child = unsafe {
                *self.arena.get_unchecked(cur as usize).children.get_unchecked(idx)
            };

            if child == 0 {
                let new_idx = self.arena.len() as u32;
                self.arena.push(TrieNode::new());
                // После push указатель на arena[cur] мог измениться (realloc),
                // поэтому используем индекс заново — не сохраняем ссылку.
                unsafe {
                    *self.arena
                        .get_unchecked_mut(cur as usize)
                        .children
                        .get_unchecked_mut(idx) = new_idx;
                }
                cur = new_idx;
            } else {
                cur = child;
            }
        }
        let node = unsafe { self.arena.get_unchecked_mut(cur as usize) };
        if node.count == 0 {
            self.unique_words += 1;
        }
        node.count += 1;
    }

    #[inline]
    fn get(&self, word: &[u8]) -> usize {
        let mut cur = 0u32;
        for &b in word {
            let idx = LUT[b as usize];
            if idx == INVALID { return 0; }
            let child = unsafe {
                *self.arena.get_unchecked(cur as usize).children.get_unchecked(idx as usize)
            };
            if child == 0 { return 0; }
            cur = child;
        }
        unsafe { self.arena.get_unchecked(cur as usize).count as usize }
    }

    #[inline(always)]
    fn nodes(&self) -> usize { self.arena.len() }

    #[inline(always)]
    fn size(&self) -> usize { self.unique_words }
}

// ─────────────────────────────────────────────────────────────────────────────
// Совмещённый парсинг + вставка в Trie за ОДИН проход.
//
// Ключевое отличие от предыдущей версии: мы НЕ формируем срез &[u8] слова
// и не вызываем insert(slice). Вместо этого мы идём по символам прямо
// внутри build_trie и переходим по узлам дерева на лету — без промежуточного
// буфера и без второго прохода по слову.
// ─────────────────────────────────────────────────────────────────────────────
fn build_trie(text: &[u8]) -> (Trie, Duration) {
    let t0 = Instant::now();
    let mut trie = Trie::with_capacity(5_000_000);

    let mut cur = 0u32;          // текущий узел
    let mut in_word = false;

    for &b in text {
        let idx = LUT[b as usize];
        if idx != INVALID {
            // ── буква: продвигаемся по дереву ────────────────────────────
            let idx = idx as usize;
            let child = unsafe {
                *trie.arena.get_unchecked(cur as usize).children.get_unchecked(idx)
            };
            if child == 0 {
                let new_idx = trie.arena.len() as u32;
                trie.arena.push(TrieNode::new());
                unsafe {
                    *trie.arena
                        .get_unchecked_mut(cur as usize)
                        .children
                        .get_unchecked_mut(idx) = new_idx;
                }
                cur = new_idx;
            } else {
                cur = child;
            }
            in_word = true;
        } else if in_word {
            // ── конец слова: помечаем текущий узел ───────────────────────
            let node = unsafe { trie.arena.get_unchecked_mut(cur as usize) };
            if node.count == 0 { trie.unique_words += 1; }
            node.count += 1;
            cur = 0;
            in_word = false;
        }
    }
    // последнее слово (если файл не заканчивается разделителем)
    if in_word {
        let node = unsafe { trie.arena.get_unchecked_mut(cur as usize) };
        if node.count == 0 { trie.unique_words += 1; }
        node.count += 1;
    }

    (trie, t0.elapsed())
}

// ─────────────────────────────────────────────────────────────────────────────
// HashMap-baseline (без изменений)
// ─────────────────────────────────────────────────────────────────────────────
fn build_hashmap(text: &[u8]) -> (HashMap<&[u8], usize>, Duration) {
    let t0 = Instant::now();
    let mut dict: HashMap<&[u8], usize> = HashMap::new();

    let n = text.len();
    let mut start = 0usize;
    let mut in_word = false;

    for i in 0..n {
        let is_letter = LUT[text[i] as usize] != INVALID;
        if is_letter {
            if !in_word { start = i; in_word = true; }
        } else if in_word {
            *dict.entry(&text[start..i]).or_insert(0) += 1;
            in_word = false;
        }
    }
    if in_word {
        *dict.entry(&text[start..n]).or_insert(0) += 1;
    }
    (dict, t0.elapsed())
}

// ─────────────────────────────────────────────────────────────────────────────
// Проверка корректности
// ─────────────────────────────────────────────────────────────────────────────
fn verify(trie: &Trie, map: &HashMap<&[u8], usize>, probe_word: &[u8]) -> bool {
    if trie.size() != map.len() {
        eprintln!("FAIL: unique words trie={} map={}", trie.size(), map.len());
        return false;
    }
    let trie_cnt = trie.get(probe_word);
    let map_cnt  = *map.get(probe_word).unwrap_or(&0);
    if trie_cnt != map_cnt {
        eprintln!("FAIL: '{}' trie={} map={}",
                  std::str::from_utf8(probe_word).unwrap_or("?"), trie_cnt, map_cnt);
        return false;
    }
    for (&word, &cnt) in map.iter() {
        if trie.get(word) != cnt {
            eprintln!("FAIL mismatch for '{}'", std::str::from_utf8(word).unwrap_or("?"));
            return false;
        }
    }
    true
}

// ─────────────────────────────────────────────────────────────────────────────
// Вспомогательные функции вывода
// ─────────────────────────────────────────────────────────────────────────────
fn fmt_times(times: &[f64]) -> String {
    let vals: Vec<String> = times.iter().map(|t| format!("{:.5}", t)).collect();
    format!("{{{}}}", vals.join(", "))
}
fn mean(v: &[f64]) -> f64 { v.iter().sum::<f64>() / v.len() as f64 }

// ─────────────────────────────────────────────────────────────────────────────
// main
// ─────────────────────────────────────────────────────────────────────────────
fn main() {
    let args: Vec<String> = std::env::args().collect();
    let filename   = args.get(1).map(|s| s.as_str()).unwrap_or("engwiki-ascii-20260201_1gb.txt");
    let probe_word = args.get(2).map(|s| s.as_str()).unwrap_or("Dubna");
    let runs: usize = args.get(3).and_then(|s| s.parse().ok()).unwrap_or(10);

    println!("File      : {}", filename);
    println!("Probe word: {}", probe_word);
    println!("Runs      : {}", runs);
    println!();

    print!("Loading file... ");
    let t_load = Instant::now();
    let text = match fs::read(filename) {
        Ok(b) => b,
        Err(e) => { eprintln!("Cannot open '{}': {}", filename, e); std::process::exit(1); }
    };
    println!("{:.2} s  ({} MB)", t_load.elapsed().as_secs_f64(), text.len() >> 20);
    println!();

    print!("Warming up... ");
    let _ = build_trie(&text);
    let _ = build_hashmap(&text);
    println!("done\n");

    let mut hash_times = Vec::with_capacity(runs);
    let mut trie_times = Vec::with_capacity(runs);
    let mut last_info  = (0usize, 0usize, 0usize);

    for run in 0..runs {
        print!("Run {:2}/{} ... ", run + 1, runs);
        let (map,  ht) = build_hashmap(&text);
        let (trie, tt) = build_trie(&text);

        if run == 0 {
            let ok = verify(&trie, &map, probe_word.as_bytes());
            println!("hash={:.5}s  trie={:.5}s  verify={}",
                     ht.as_secs_f64(), tt.as_secs_f64(), if ok {"OK"} else {"FAIL"});
            if !ok { std::process::exit(2); }
        } else {
            println!("hash={:.5}s  trie={:.5}s", ht.as_secs_f64(), tt.as_secs_f64());
        }

        last_info = (trie.nodes(), trie.size(), trie.get(probe_word.as_bytes()));
        hash_times.push(ht.as_secs_f64());
        trie_times.push(tt.as_secs_f64());
    }

    println!();
    println!("=== Results ===");
    println!("hash: {}", fmt_times(&hash_times));
    println!("trie: {}", fmt_times(&trie_times));
    println!("Среднее ускорение: {:.5}x", mean(&hash_times) / mean(&trie_times));
    println!();
    println!("Trie nodes  : {}", last_info.0);
    println!("Unique words: {}", last_info.1);
    println!("'{}' count  : {}", probe_word, last_info.2);

    let mut csv = String::from("run,method,seconds\n");
    for (i, (&h, &t)) in hash_times.iter().zip(trie_times.iter()).enumerate() {
        csv.push_str(&format!("{},hash,{}\n", i + 1, h));
        csv.push_str(&format!("{},trie,{}\n", i + 1, t));
    }
    fs::write("benchmark_results.csv", &csv).expect("Cannot write CSV");
    println!();
    println!("Benchmark data saved to 'benchmark_results.csv'");
    println!("Run 'python3 plot_benchmark.py' to generate the boxplot.");
}
