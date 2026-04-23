use std::collections::HashMap;
use std::hash::{DefaultHasher, Hash, Hasher};

struct Node {
    name: char,
    children: HashMap<char, usize>,
}

struct Tree {
    nodes: Vec<Node>,
}

impl Tree {
    fn new() -> Self {
        let root = Node {
            name: '/',
            children: HashMap::new(),
        };
        Tree { nodes: vec![root] }
    }

    fn add_path(&mut self, path: &str) {
        let mut cur = 0;
        for c in path.chars() {
            if c == '/' {
                continue;
            }
            if let Some(&next) = self.nodes[cur].children.get(&c) {
                cur = next;
            } else {
                let new_idx = self.nodes.len();
                self.nodes[cur].children.insert(c, new_idx);
                self.nodes.push(Node {
                    name: c,
                    children: HashMap::new(),
                });
                cur = new_idx;
            }
        }
    }

    fn count_duplicate_subtrees(&self) -> usize {
        let n = self.nodes.len();
        let mut subtree_hash: Vec<u64> = vec![0; n];
        let mut freq: HashMap<u64, usize> = HashMap::new();
        let mut stack: Vec<(usize, bool)> = vec![(0, false)];

        while let Some((v, done)) = stack.last().copied() {
            if !done {
                stack.last_mut().unwrap().1 = true;
                for &child in self.nodes[v].children.values() {
                    stack.push((child, false));
                }
            } else {
                stack.pop();

                let mut child_hashes: Vec<u64> = self.nodes[v]
                    .children
                    .values()
                    .map(|&c| subtree_hash[c])
                    .collect();
                child_hashes.sort_unstable();

                let mut h = DefaultHasher::new();
                self.nodes[v].name.hash(&mut h);
                child_hashes.hash(&mut h);
                let hash = h.finish();

                subtree_hash[v] = hash;
                *freq.entry(hash).or_insert(0) += 1;
            }
        }

        freq.iter()
            .filter(|&(&k, &c)| k != subtree_hash[0] && c >= 2)
            .count()
    }
}

fn main() {
    let mut tree = Tree::new();
    let stdin = std::io::stdin();
    let mut line = String::new();

    loop {
        line.clear();
        stdin.read_line(&mut line).unwrap();
        let trimmed = line.trim();
        if trimmed.is_empty() {
            break;
        }
        if trimmed.starts_with('/') {
            tree.add_path(trimmed);
        }
    }

    println!("{}", tree.count_duplicate_subtrees());
}
