use mesh::*;
use std::iter::*;

use vertex::*;

pub struct ProcessableMesh {
    edges: Vec<Vec<u32>>,
    triangles_for_vert: Vec<Vec<u32>>,

    vertices: Vec<Vertex>,
    triangles: Vec<[u32; 3]>
}

impl ProcessableMesh {
    pub fn from_mesh(mesh: &Mesh) -> ProcessableMesh {
        let triangles = mesh.indices.iter().map(|t| [t.0, t.1, t.2]).collect::<Vec<_>>();
        ProcessableMesh {
            edges: compute_edges(&triangles, mesh.vertices.len()),
            triangles_for_vert: compute_triangles(&triangles, mesh.vertices.len()),

            vertices: mesh.vertices.clone(),
            triangles: triangles,
        }
    }

    pub fn vertex_move(&mut self) {
        for i in 0..self.edges.len() {
            if self.is_boundary(i) {
                continue;
            }
            let mut p = Vec3::default();
            for neighbor in &self.edges[i] {
                p += self.vertices[*neighbor as usize].position;
            }
            self.vertices[i].position = p / (self.edges[i].len() as f32);
        }
    }

    pub fn remesh(&mut self) {
        {
            let mut flipped = 0;
            let len = self.edges.len();
            for i in 0..len {
                //println!("\n{:?}", i);
                if self.edges[i].len() > 0 {
                    let edge = [i as u32, self.edges[i][0]];
                    if self.flip_edge(&edge) {
                        flipped += 1;
                    }
                }
            }
            println!("{:?} edges flipped.", flipped);
        }
        //self.vertex_move();
    }

    fn flip_edge(&mut self, edge: &[u32; 2]) -> bool {
        /*if self.is_boundary(edge[0] as usize) && self.is_boundary(edge[1] as usize) {
            return false;
        }*/
        let tris = self.triangles_for_edge(edge);
        if tris.len() == 2 {
            let ti1 = tris[0] as usize;
            let ti2 = tris[1] as usize;
            let t1 = self.triangles[ti1];
            let t2 = self.triangles[ti2];


            if self.is_optuce(&t1) || self.is_optuce(&t2) || degenerate(&t1) || degenerate(&t2) {
                println!("{} {} {} {}", self.is_optuce(&t1), self.is_optuce(&t2), degenerate(&t1),  degenerate(&t2));
                return false;
            }

            let a_t = third_point(&t1, &edge);
            let b_t = third_point(&t2, &edge);

            if a_t == b_t || self.edges[a_t as usize].contains(&b_t) {
                println!("flubudu");
                return false;
            }

            assert!(edge[0] != a_t);
            assert!(edge[1] != a_t);
            assert!(edge[0] != b_t);
            assert!(edge[1] != b_t);
            assert!(a_t != b_t);

            self.triangles[ti1] = [b_t, a_t, edge[0]];
            self.triangles[ti2] = [a_t, b_t, edge[1]];

            remove(&mut self.edges[edge[0] as usize], edge[1]);
            remove(&mut self.edges[edge[1] as usize], edge[0]);
            self.edges[a_t as usize].push(b_t);
            self.edges[b_t as usize].push(a_t);

            remove(&mut self.triangles_for_vert[edge[0] as usize], tris[1]);
            remove(&mut self.triangles_for_vert[edge[1] as usize], tris[0]);

            if self.triangles_for_vert[a_t as usize].contains(&(ti2 as u32)) ||
               self.triangles_for_vert[b_t as usize].contains(&(ti1 as u32)) {
                println!("{:?}, {:?}", self.triangles_for_vert[a_t as usize], self.triangles_for_vert[b_t as usize]);
                println!("{:?}, {:?}", ti1, ti2);
                panic!("Invalid remeshing operation.")
            }

            self.triangles_for_vert[a_t as usize].push(ti2 as u32);
            self.triangles_for_vert[b_t as usize].push(ti1 as u32);

            true
        } else {
            println!("???? {:?}", tris);
            false
        }
    }

    fn is_optuce(&self, tri: &[u32; 3]) -> bool {
        let a = self.vertices[tri[0] as usize].position;
        let b = self.vertices[tri[1] as usize].position;
        let c = self.vertices[tri[2] as usize].position;
        let e1 = (a - b).normalized();
        let e2 = (b - c).normalized();
        let e3 = (a - c).normalized();

        //let epsilon = 0.001;
        //println!("{:?}, {:?}", e1.dot(&e2), e1.dot(&e3));
        e1.dot(&e2) > 0.0 || e1.dot(&e3) < 0.0
    }

    pub fn to_mesh(&self) -> Mesh {
        Mesh {
            vertices: self.vertices.clone(),
            indices: self.triangles.iter().map(|t| (t[0], t[1], t[2])).collect::<Vec<_>>()
        }
    }

    fn is_boundary(&self, index: usize) -> bool {
        let neighbors = &self.edges[index];
        for n in neighbors {
            if self.edges[*n as usize].iter().fold(0,
                        |count, vert| count + (neighbors.iter().any(|e| e == vert) as usize)
                    ) == 1 {
                return true;
            }
        }
        false
    }



    fn triangles_for_edge(&self, edge: &[u32; 2]) -> Vec<u32> {
        let ref a_t = self.triangles_for_vert[edge[0] as usize];
        let ref b_t = self.triangles_for_vert[edge[1] as usize];
        let tris = a_t.iter().filter(|t| b_t.contains(t)).cloned().collect::<Vec<_>>();
        if tris.len() > 2 {
            println!("{:?} & {:?} -> {:?}", a_t, b_t, tris);
            panic!("Mesh is not manifold.");
        }
        tris
    }
}

fn remove(vec: &mut Vec<u32>, val: u32) {
    if let Some(pos) = vec.iter().position(|x| *x == val) {
        vec.swap_remove(pos);
    } else {
        panic!("Element not found.");
    }
}

fn degenerate(t: &[u32; 3]) -> bool {
    t[0] == t[1] || t[0] == t[2] || t[1] == t[2]
}

fn third_point(t: &[u32; 3], e: &[u32; 2]) -> u32 {
    'next: for i in 0..3 {
        for j in 0..2 {
            if t[i] == e[j] {
                continue 'next;
            }
        }
        return t[i];
    }
    println!("{:?} in {:?}", e, t);
    panic!("Unable to find point in triangle.");
}

/*fn reversed(tri: &[u32; 3], edge: &[u32; 2]) -> bool {
    let a = edge[0];
    let b = edge[1];

    (tri[0] == b) ||
    (tri[2] == a) ||
    (tri[1] == b && tri[2] == a)
}*/

/*fn common_edge(a: &[u32; 3], b: &[u32; 3]) -> Option<[u32; 2]> {
    for i in 0..3 {
        for j in i..3 {
            if a[i] == b[j] {
                for k in (i + 1)..3 {
                    for l in 0..3 {
                        if a[k] == b[l] {
                            return Some([a[i], a[k]]);
                        }
                    }
                }
            }
        }
    }
    None
}*/

fn compute_edges(tris: &Vec<[u32; 3]>, vertices: usize) -> Vec<Vec<u32>> {
    println!("edges");
    let mut edges = Vec::new();
    edges.resize(vertices, Vec::new());
    for tri in tris {
        for i in 0..3 {
            for j in 0..3 {
                if i != j {
                    add_not_exists(tri[i], &mut edges[tri[j] as usize]);
                }
            }
        }
    }
    /*for mut e in edges.iter_mut() {
        e.sort();
    }*/
    edges
}

fn add_not_exists(val: u32, vec: &mut Vec<u32>) {
    if !vec.contains(&val) {
        vec.push(val);
    }
}

fn compute_triangles(tris: &Vec<[u32; 3]>, vertices: usize) -> Vec<Vec<u32>> {
    let mut for_verts = Vec::new();
    for_verts.resize(vertices, Vec::new());

    for (i, tri) in tris.iter().enumerate() {
        for v in 0..3 {
            add_not_exists(i as u32, &mut for_verts[tri[v] as usize]);
        }
    }
    for_verts
}
