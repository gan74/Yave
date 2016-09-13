use std::io::{BufReader, Read, BufRead, Error, ErrorKind};
use std::collections::HashMap;
use std::str::FromStr;
use std::fmt::Debug;
use std::error;
use std::io;

use loader::*;
use mesh::*;
use vertex::*;


pub struct ObjLoader {
}

impl ObjLoader {
    pub fn new() -> ObjLoader {
        ObjLoader {
        }
    }
}

#[derive(PartialEq, Eq, Hash, Clone, Copy, Debug)]
struct Index {
    pos: usize,
    uv: usize,
    norm: usize
}

impl Loader for ObjLoader {
    fn extentions(&self) -> Vec<&'static str> {
        vec![".obj"]
    }

    fn load(&self, file: &mut Read) -> io::Result<Mesh> {
        let mut sm = LoaderStateMachine::new();

        for line in BufReader::new(file).lines() {
            let line = try!(line);
            try!(sm.process_line(line.as_str()));
        }

        Ok(sm.finalize())
    }
}

struct LoaderStateMachine {
    index_map: HashMap<Index, u32>,
    uvs: Vec<Uv>,
    normals: Vec<Normal>,
    positions: Vec<Position>,
    indices: Vec<(u32, u32, u32)>,
    vertices: Vec<Vertex>,
    smoothing: bool
}

impl LoaderStateMachine {
    pub fn new() -> LoaderStateMachine {
        LoaderStateMachine {
            index_map: HashMap::new(),
            uvs: Vec::new(),
            normals: Vec::new(),
            positions: Vec::new(),
            indices: Vec::new(),
            vertices: Vec::new(),
            smoothing: false
        }
    }

    pub fn finalize(mut self) -> Mesh {
        self.renorm_all();

        Mesh {
            vertices: self.vertices,
            indices: self.indices
        }
    }

    fn renorm_all(&mut self) {
        for tri in self.indices.iter() {
            let norm = self.compute_normal(tri.clone());
            renorm(&mut self.vertices[tri.0 as usize], norm);
            renorm(&mut self.vertices[tri.1 as usize], norm);
            renorm(&mut self.vertices[tri.2 as usize], norm);
        }
    }

    pub fn process_line(&mut self, line: &str) -> io::Result<()> {
        let trimmed = line.trim_left();
        if trimmed.starts_with("vn ") {
            self.process_normal(&trimmed[3..])
        } else if trimmed.starts_with("vt ") {
            self.process_uvs(&trimmed[3..])
        } else if trimmed.starts_with("s ") {
            self.smoothing = match &trimmed[2..] {
                "0" | "off" | "false" => false,
                _ => true
            };
            Ok(())
        } else if trimmed.starts_with("v ") {
            self.process_position(&trimmed[2..])
        } else if trimmed.starts_with("f ") {
            self.process_face(&trimmed[2..])
        } else {
            Ok(())
        }
    }

    fn smooth_triangle(&mut self, tri: (u32, u32, u32)) {
        let norm = self.compute_normal(tri);
        self.vertices[tri.0 as usize].normal += norm;
        self.vertices[tri.1 as usize].normal += norm;
        self.vertices[tri.2 as usize].normal += norm;
    }

    fn compute_normal(&self, tri: (u32, u32, u32)) -> Normal {
        let edge_0 = self.vertices[tri.1 as usize].position - self.vertices[tri.0 as usize].position;
        let edge_1 = self.vertices[tri.1 as usize].position - self.vertices[tri.2 as usize].position;
        edge_0.cross(edge_1)
    }

    fn process_normal(&mut self, line: &str) -> io::Result<()> {
        let v = try!(parse_vec(line));
        self.normals.push(Vec3(v[0], v[1], v[2]));
        Ok(())
    }

    fn process_position(&mut self, line: &str) -> io::Result<()> {
        let v = try!(parse_vec(line));
        self.positions.push(Vec3(v[0], v[1], v[2]));
        Ok(())
    }

    fn process_uvs(&mut self, line: &str) -> io::Result<()> {
        let v = try!(parse_vec(line));
        self.uvs.push(Uv(v[0], v[1]));
        Ok(())
    }

    fn process_face(&mut self, line: &str) -> io::Result<()> {
        for mut tri in triangulate(try!(parse_face(line))) {
            if self.smoothing {
                tri.0.norm = 0;
                tri.1.norm = 0;
                tri.2.norm = 0;
            }

            let a = try!(self.find_vertex(tri.0));
            let b = try!(self.find_vertex(tri.1));
            let c = try!(self.find_vertex(tri.2));
            self.indices.push((a, b, c));

            if self.smoothing {
                self.smooth_triangle((a, b, c));
            }
        }
        Ok(())
    }

    fn find_vertex(&mut self, k: Index) -> io::Result<u32> {
        if let Some(index) = self.index_map.get(&k).cloned() {
            Ok(index)
        } else {
            let index = self.vertices.len() as u32;
            self.index_map.insert(k, index);
            let norm = if self.smoothing {
                Normal::default()
            } else {
                try!(check_bound(k.norm, &self.normals))
            };
            self.vertices.push(Vertex {
                position: try!(check_bound(k.pos, &self.positions)),
                normal: norm,
                uv: try!(check_bound(k.uv, &self.uvs))
            });
            Ok(index)
        }
    }
}


fn parse_vec<T: FromStr>(line: &str) -> io::Result<Vec<T>>
    where <T as FromStr>::Err: Send + Sync + error::Error + 'static
{
    let mut v = Vec::new();
    for i in line.split_whitespace() {
        match i.parse::<T>() {
            Ok(i) => v.push(i),
            Err(err) => return Err(Error::new(ErrorKind::InvalidData, err))
        }
    }
    Ok(v)
}

fn triangulate(indices: Vec<Index>) -> Vec<(Index, Index, Index)> {
    match indices.len() {
        3 => vec![(indices[0], indices[1], indices[2])],
        4 => vec![(indices[0], indices[1], indices[2]), (indices[0], indices[2], indices[3])],
        _ => panic!("invalid face")
    }
}

fn parse_face(line: &str) -> io::Result<Vec<Index>> {
    let mut v = Vec::new();
    for i in line.split_whitespace() {
        let indices = i.split('/').map(|i| i.parse::<usize>().unwrap_or(0)).collect::<Vec<_>>();
        match indices.len() { // position/uv/normal
            1 => v.push(Index{pos: indices[0], norm: 0, uv: 0}),
            2 => v.push(Index{pos: indices[0], norm: 0, uv: indices[1]}),
            3 => v.push(Index{pos: indices[0], norm: indices[2], uv: indices[1]}),
            _ => return Err(Error::new(ErrorKind::InvalidData, "invalid attribute count"))
        }
    }
    Ok(v)
}

fn check_bound<T: Clone + Default + Debug>(index: usize, vec: &Vec<T>) -> io::Result<T> {
    if index == 0 {
        Ok(Default::default())
    } else if index <= vec.len() {
        Ok(vec[index - 1].clone())
    } else {
        Err(Error::new(ErrorKind::InvalidData, "invalid vertex index"))
    }
}

fn renorm(vert: &mut Vertex, norm: Normal) {
    if vert.normal.length2() < 0.00001 {
        vert.normal = norm;
    }
    vert.normal.normalize();
}
