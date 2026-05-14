use std::collections::HashMap;
use std::env;
use std::fs::File;
use std::io::{Read, Write, BufReader, BufWriter};
use std::time::Instant;

const MAGIC_NUMBER: &[u8; 4] = b"LZ7B"; // Changed magic number for bit-packed version

/// Helper to write bits to a byte stream
struct BitWriter<W: Write> {
    writer: W,
    buffer: u64,
    bits_in_buffer: u32,
}

impl<W: Write> BitWriter<W> {
    fn new(writer: W) -> Self {
        BitWriter { writer, buffer: 0, bits_in_buffer: 0 }
    }

    fn write_bits(&mut self, value: u32, count: u32) {
        self.buffer |= (value as u64) << self.bits_in_buffer;
        self.bits_in_buffer += count;
        while self.bits_in_buffer >= 8 {
            self.writer.write_all(&[(self.buffer & 0xFF) as u8]).unwrap();
            self.buffer >>= 8;
            self.bits_in_buffer -= 8;
        }
    }

    fn flush(&mut self) {
        if self.bits_in_buffer > 0 {
            self.writer.write_all(&[(self.buffer & 0xFF) as u8]).unwrap();
        }
        self.writer.flush().unwrap();
    }
}

/// Helper to read bits from a byte stream
struct BitReader<R: Read> {
    reader: R,
    buffer: u64,
    bits_in_buffer: u32,
}

impl<R: Read> BitReader<R> {
    fn new(reader: R) -> Self {
        BitReader { reader, buffer: 0, bits_in_buffer: 0 }
    }

    fn read_bits(&mut self, count: u32) -> Option<u32> {
        while self.bits_in_buffer < count {
            let mut byte = [0u8; 1];
            if self.reader.read_exact(&mut byte).is_err() {
                return None;
            }
            self.buffer |= (byte[0] as u64) << self.bits_in_buffer;
            self.bits_in_buffer += 8;
        }
        let value = (self.buffer & ((1 << count) - 1)) as u32;
        self.buffer >>= count;
        self.bits_in_buffer -= count;
        Some(value)
    }
}

fn main() {
    let args: Vec<String> = env::args().collect();
    if args.len() < 4 {
        print_help();
        return;
    }

    match args[1].as_str() {
        "-c" => compress(&args[2], &args[3]),
        "-d" => decompress(&args[2], &args[3]),
        _ => print_help(),
    }
}

fn compress(input_path: &str, output_path: &str) {
    let mut input_file = File::open(input_path).expect("Input not found");
    let mut buffer = Vec::new();
    input_file.read_to_end(&mut buffer).unwrap();

    // Check ASCII
    if buffer.iter().any(|&b| b > 127) {
        println!("Fail! The input file must be ASCII with 128 characters only!");
        return;
    }

    let start = Instant::now();
    let out_file = File::create(output_path).unwrap();
    let mut bw = BitWriter::new(BufWriter::new(out_file));

    // Write Header
    bw.writer.write_all(MAGIC_NUMBER).unwrap();

    let mut dictionary: HashMap<Vec<u8>, u32> = HashMap::new();
    let mut current = Vec::new();
    let mut next_idx = 1;

    for &byte in &buffer {
        let mut combined = current.clone();
        combined.push(byte);

        if dictionary.contains_key(&combined) {
            current = combined;
        } else {
            let prefix_idx = if current.is_empty() { 0 } else { *dictionary.get(&current).unwrap() };

            // Optimization: Calculate bits needed for the current dictionary size
            let bits_needed = 32 - (next_idx as u32).leading_zeros().max(1);

            bw.write_bits(prefix_idx, bits_needed);
            bw.write_bits(byte as u32, 7); // Use 7 bits for ASCII

            dictionary.insert(combined, next_idx);
            next_idx += 1;
            current.clear();
        }
    }

    // Handle last sequence
    if !current.is_empty() {
        let last = current.pop().unwrap();
        let prefix_idx = if current.is_empty() { 0 } else { *dictionary.get(&current).unwrap() };
        let bits_needed = 32 - (next_idx as u32).leading_zeros().max(1);
        bw.write_bits(prefix_idx, bits_needed);
        bw.write_bits(last as u32, 7);
    }

    bw.flush();

    let duration = start.elapsed();
    let original = buffer.len() as f64;
    let compressed = std::fs::metadata(output_path).unwrap().len() as f64;
    println!("Done! Ratio: {:.2}. Time: {:.3}s", original / compressed, duration.as_secs_f64());
}

fn decompress(input_path: &str, output_path: &str) {
    let in_file = File::open(input_path).expect("Archive not found");
    let start = Instant::now();
    let mut br = BitReader::new(BufReader::new(in_file));

    let mut header = [0u8; 4];
    if br.reader.read_exact(&mut header).is_err() || &header != MAGIC_NUMBER {
        println!("Fail! It is not a .compressed archive!");
        return;
    }

    let mut dictionary: Vec<Vec<u8>> = vec![vec![]];
    let mut result = Vec::new();
    let mut next_idx = 1;

    loop {
        let bits_needed = 32 - (next_idx as u32).leading_zeros().max(1);

        let index = match br.read_bits(bits_needed) {
            Some(idx) => idx as usize,
            None => break, // Natural end of bitstream
        };

        let char_code = match br.read_bits(7) {
            Some(c) => c as u8,
            None => break,
        };

        if index >= dictionary.len() {
            println!("Fail! The archive is corrupt!");
            return;
        }

        let mut entry = dictionary[index].clone();
        entry.push(char_code);
        result.extend(&entry);
        dictionary.push(entry);
        next_idx += 1;
    }

    File::create(output_path).unwrap().write_all(&result).unwrap();
    println!("Done! Time: {:.3}s", start.elapsed().as_secs_f64());
}

fn print_help() {
    println!("LZ78 Bit-Packed Compressor\nUsage: -c input output | -d archive output");
}
