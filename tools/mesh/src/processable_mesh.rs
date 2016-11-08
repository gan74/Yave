use mesh::*;
use std::iter::*;
use std::cmp;

use vertex::*;

pub struct ProcessableMesh {
    pub mesh: Mesh,
    edges: Vec<Vec<u32>>
}

impl ProcessableMesh {
    pub fn from_mesh(mesh: &Mesh) -> ProcessableMesh {
        ProcessableMesh {
            mesh: mesh.clone(),
            edges: compute_edges(&mesh.indices, mesh.vertices.len())
        }
    }

    pub fn print_info(&self) {
        let mut val = 0;
        let mut max = 0;
        for v in &self.edges {
            val += v.len();
            max = cmp::max(max, v.len())
        }
        println!("average valence = {}", val as f32 / self.edges.len() as f32);
        println!("maximum valence = {}", max);
    }

    pub fn vertex_move(&mut self) {
        for i in 0..self.edges.len() {
            if self.is_boundary(i) {
                continue;
            }
            let mut p = Vec3::default();
            for neighbor in &self.edges[i] {
                p += self.mesh.vertices[*neighbor as usize].position;
            }
            self.mesh.vertices[i].position = p / (self.edges[i].len() as f32);
        }
    }

    fn is_boundary(&self, index: usize) -> bool {
        let neighbors = &self.edges[index];
        for n in neighbors {
            if self.edges[*n as usize].iter().any(|vert| !self.edges[*vert as usize].iter().any(|e| e == n)) {
                return false;
            }
        }
        true
    }
}

fn compute_edges(tris: &Vec<(u32, u32, u32)>, vertices: usize) -> Vec<Vec<u32>> {
    let mut edges = (0..vertices).map(|_| Vec::new()).collect::<Vec<_>>();
    for tri in tris {
        add_not_exists(tri.1, &mut edges[tri.0 as usize]);
        add_not_exists(tri.2, &mut edges[tri.0 as usize]);

        add_not_exists(tri.0, &mut edges[tri.1 as usize]);
        add_not_exists(tri.2, &mut edges[tri.1 as usize]);

        add_not_exists(tri.0, &mut edges[tri.2 as usize]);
        add_not_exists(tri.1, &mut edges[tri.2 as usize]);
    }
    edges
}

fn add_not_exists(val: u32, vec: &mut Vec<u32>) {
    for i in vec.iter() {
        if val == i.clone() {
            return;
        }
    }
    vec.push(val)
}
