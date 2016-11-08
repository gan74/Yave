
use vertex::*;

//use std::iter;

#[derive(PartialEq, Clone)]
pub struct Mesh {
    pub vertices: Vec<Vertex>,
    pub indices: Vec<(u32, u32, u32)>
}

/*pub struct TriangleRef<'l> {
    mesh: &'l Mesh,
    indices: [usize; 3]
}

impl<'l> TriangleRef<'l> {
    fn compute_normal(&self) -> Vec3 {
        let v0 = self.vertex(0);
        let v1 = self.vertex(1);
        let v2 = self.vertex(2);
        let edge_0 = v1.position - v0.position;
        let edge_1 = v1.position - v2.position;
        edge_0.cross(edge_1)
    }

    fn vertex(&self, i: usize) -> &Vertex {
        &self.mesh.vertices[self.indices[i]]
    }
}*/

impl Mesh {
    pub fn reverse_faces(&mut self) {
        for tri in self.indices.iter_mut() {
            *tri = (tri.2, tri.1, tri.0);
        }
    }

    pub fn scale(&mut self, scale: Vec3) {
        for vert in self.vertices.iter_mut() {
            vert.position *= scale;
            vert.normal *= scale;
        }
    }

    /*pub fn recompute_normals(&mut self) {
        for mut v in self.vertices.iter_mut() {
            v.normal = Vec3::default();
        }
        for tri in self.triangles() {
            let n = tri.compute_normal();
            for i in 0..3 {
                self.vertices[tri.indices[i]].normal += n;
            }
        }
    }

    pub fn triangles<'a>(&'a self) -> Box<iter::Iterator<Item = TriangleRef> + 'a> {
        Box::new(self.indices.iter().map(move |t| TriangleRef {
            mesh: &self,
            indices: [t.0 as usize, t.1 as usize, t.2 as usize]
        }))
    }*/
}
