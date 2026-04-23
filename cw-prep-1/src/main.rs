use std::collections::{HashMap, VecDeque};
use std::time::Instant;

struct Node {
    name: char,
    children: HashMap<char, usize>,
    parent: Option<usize>,
    depth: usize
}

struct Tree {
    nodes: Vec<Node>
}

impl Tree {
    fn new() -> Self {
        let root = Node {
            name: '/',
            children: HashMap::new(),
            parent: None,
            depth: 0
        };
        Tree { nodes: vec![root] }
    }

    fn add_path(&mut self, path: &str) {
        let mut current_idx = 0;                        // start at root
        for c in path.chars() {
            if c == '/' {
                continue;
            }
            if let Some(&next_idx) = self.nodes[current_idx].children.get(&c) {
                current_idx = next_idx;
            } else {
                let new_idx = self.nodes.len();
                let new_depth = self.nodes[current_idx].depth + 1;
                self.nodes[current_idx].children.insert(c, new_idx);

                self.nodes.push(Node {
                    name: c,
                    children: HashMap::new(),
                    parent: Some(current_idx),
                    depth: new_depth
                });
                current_idx = new_idx;
            }
        }
    }

    // Helper to get all indices of nodes with a specific name
    fn get_indices(&self, name: char) -> Vec<usize> {
        self.nodes.iter().enumerate()
            .filter(|(_, n)| n.name == name)
            .map(|(i, _)| i)
            .collect()
    }

    // Generic BFS that returns distances from a set of starting nodes
    fn bfs(&self, start_nodes: &[usize]) -> Vec<Option<usize>> {
        let mut distances = vec![None; self.nodes.len()];
        let mut queue = VecDeque::new();

        for &idx in start_nodes {
            distances[idx] = Some(0);
            queue.push_back(idx);
        }

        while let Some(curr) = queue.pop_front() {
            let d = distances[curr].unwrap();

            // Collect neighbors (parent + children)
            let mut neighbors = Vec::new();
            if let Some(p) = self.nodes[curr].parent { neighbors.push(p); }
            for &child in self.nodes[curr].children.values() { neighbors.push(child); }

            for n in neighbors {
                if distances[n].is_none() {
                    distances[n] = Some(d + 1);
                    queue.push_back(n);
                }
            }
        }
        distances
    }

    pub fn solve(&self, a_name: char, b_name: char) -> (usize, usize) {
        let a_nodes = self.get_indices(a_name);
        let b_nodes = self.get_indices(b_name);

        let dists_from_a = self.bfs(&a_nodes);

        // --- MIN ---
        let mut min_dist = usize::MAX;
        for &b_idx in &b_nodes {
            if let Some(d) = dists_from_a[b_idx] {
                if d < min_dist {
                    min_dist = d;
                }
            }
        }

        // --- MAX, step 1: vertex B, furthest form set of A ---
        let mut farthest_b = b_nodes[0];
        let mut farthest_b_dist = 0;
        for &b_idx in &b_nodes {
            if let Some(d) = dists_from_a[b_idx] {
                if d > farthest_b_dist {
                    farthest_b_dist = d;
                    farthest_b = b_idx;
                }
            }
        }

        // --- MAX, step 2: BFS from found B, furthest A ---
        let dists_from_farthest_b = self.bfs(&[farthest_b]);

        let mut max_dist = 0;
        for &a_idx in &a_nodes {
            if let Some(d) = dists_from_farthest_b[a_idx] {
                if d > max_dist {
                    max_dist = d;
                }
            }
        }

        (min_dist, max_dist)
    }

    // pub fn solve(&self, a_name: char, b_name: char) -> (usize, usize) {
    //     let a_nodes = self.get_indices(a_name);
    //     let b_nodes = self.get_indices(b_name);
    //
    //     // --- MIN DISTANCE ---
    //     let dists_from_a = self.bfs(&a_nodes);
    //     let mut min_dist = usize::MAX;
    //     for &b_idx in &b_nodes {
    //         if let Some(d) = dists_from_a[b_idx] {
    //             if d < min_dist { min_dist = d; }
    //         }
    //     }
    //
    //     // --- MAX DISTANCE ---
    //     // Find the deepest A and deepest B to act as extreme starting points
    //     let deepest_a = *a_nodes.iter().max_by_key(|&&i| self.nodes[i].depth).unwrap();
    //     let deepest_b = *b_nodes.iter().max_by_key(|&&i| self.nodes[i].depth).unwrap();
    //
    //     let dists_from_extreme_a = self.bfs(&[deepest_a]);
    //     let dists_from_extreme_b = self.bfs(&[deepest_b]);
    //
    //     let mut max_dist = 0;
    //     for &b_idx in &b_nodes {
    //         max_dist = max_dist.max(dists_from_extreme_a[b_idx].unwrap_or(0));
    //     }
    //     for &a_idx in &a_nodes {
    //         max_dist = max_dist.max(dists_from_extreme_b[a_idx].unwrap_or(0));
    //     }
    //
    //     (min_dist, max_dist)
    // }

    pub fn display(&self) {
        self.print_recursive(0, 0);
    }

    fn print_recursive(&self, node_idx: usize, indent: usize) {
        let node = &self.nodes[node_idx];
        println!("{:indent$}{}", "", node.name, indent = indent * 2);
        let mut sorted_keys: Vec<_> = node.children.keys().collect();
        sorted_keys.sort();
        for name in sorted_keys {
            let child_idx = node.children[name];
            self.print_recursive(child_idx, indent + 1);
        }
    }
}

fn main() {
    let mut tree = Tree::new();
    let mut input = String::new();
    let stdin = std::io::stdin();

    // 1. Read paths
    loop {
        input.clear();
        stdin.read_line(&mut input).unwrap();
        let line = input.trim();
        if line.is_empty() { break; }
        // If the line doesn't start with '/', it might be the target chars line
        if !line.starts_with('/') {
            process_targets(line, &tree);
            return;
        }
        tree.add_path(line);
    }

    tree.display();

    // 2. Read target chars (if not found in loop)
    input.clear();
    stdin.read_line(&mut input).unwrap();
    process_targets(input.trim(), &tree);
    stress_test_line(1000);
    stress_test_line(10000);
    stress_test_line(100000);
}

fn process_targets(line: &str, tree: &Tree) {
    let chars: Vec<char> = line.chars().filter(|c| !c.is_whitespace()).collect();
    if chars.len() >= 2 {
        let (min, max) = tree.solve(chars[0], chars[1]);
        println!("Min: {}, Max: {}", min, max);
    }
}


fn stress_test_line(n: usize) {
    let mut tree = Tree::new();
    let long_path = "/".to_string() + &"a/".repeat(n);

    let start = Instant::now();
    tree.add_path(&long_path);
    let build_time = start.elapsed();

    let start_solve = Instant::now();
    let (min, max) = tree.solve('a', 'a');
    let solve_time = start_solve.elapsed();

    println!("N={}: Build: {:?}, Solve: {:?}", n, build_time, solve_time);
}
