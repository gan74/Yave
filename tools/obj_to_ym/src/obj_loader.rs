
use std::io::{BufRead};
use std::str::{FromStr};
use std::collections::{HashMap};

use types::*;



pub fn load_obj(file: &mut BufRead) -> Result<Mesh, Error> {
	let mut positions = Vec::new();
	let mut normals = Vec::new();
	let mut uvs = Vec::new();

	positions.push([0.0, 0.0, 0.0]);
	normals.push([0.0, 0.0, 0.0]);
	uvs.push([0.0, 0.0]);

	let mut vertices = Vec::new();
	let mut triangles = Vec::new();
	let mut vertices_map = HashMap::new();

	let mut smoothing = false;

	for line in file.lines() {
		let line = line?;
		if line.starts_with("f ") {
			let vert = parse_3::<VertIndices>(&line[2..])?;

			/*if let Some(n) = missing_normal(&positions, &vert).map(|n| { normals.push(n); normals.len() - 1 }) {
				for i in 0..3 {
					vert[i].indices[2] = n;
				}
			}*/

			let mut tri = [0u32, 0u32, 0u32];
			for i in 0..3 {
				let indices = vert[i].indices;
				tri[i] = 
					if let Some(v) = vertices_map.get(&indices).cloned() {
						v
					} else {
						let len = vertices.len() as u32;
						vertices.push(Vertex { 
								position: positions[indices[0]],
								normal: normals[indices[2]],
								uv: uvs[indices[1]]
							});
						vertices_map.insert(indices, len);
						len
					}
			}
			if smoothing {
				smooth(&mut vertices, &tri);
			}
			triangles.push(tri);
		} else if line.starts_with("v ") {
			positions.push(parse_3::<f32>(&line[2..])?);
		} else if line.starts_with("vn ") {
			normals.push(normalize(&parse_3::<f32>(&line[3..])?));
		} else if line.starts_with("vt ") {
			uvs.push(parse_2::<f32>(&line[3..])?);
		} else if line.starts_with("s ") {
			smoothing = line[2..].parse::<usize>().unwrap_or(1) != 0;
			println!("{:?}", smoothing);
		}
	}
	
	for ref mut vert in &mut vertices {
		vert.normal = normalize(&vert.normal);
	}

	Ok(Mesh {
		vertices: vertices,
		triangles: triangles
	})
}


fn parse_3<T: FromStr>(line: &str) -> Result<[T; 3], Error> {
	let mut coords = line.split_whitespace().map(|x| x.parse::<T>());
	match (coords.next(), coords.next(), coords.next()) {
		(Some(Ok(x)), Some(Ok(y)), Some(Ok(z))) => Ok([x, y, z]),
		_ => Err(Error::from("Invalid vector."))
	}
}

fn parse_2<T: FromStr>(line: &str) -> Result<[T; 2], Error> {
	let mut coords = line.split_whitespace().map(|x| x.parse::<T>());
	match (coords.next(), coords.next()) {
		(Some(Ok(x)), Some(Ok(y))) => Ok([x, y]),
		_ => Err(Error::from("Invalid vector."))
	}
}



fn smooth(verts: &mut Vec<Vertex>, tri: &[u32; 3]) {
	let norm = compute_normal(verts, tri);
	for i in tri {
		let i = *i as usize;
		verts[i].normal = add(&verts[i].normal, &norm);
	}
}

fn compute_normal(verts: &Vec<Vertex>, tri: &[u32; 3]) -> Vec3 {
    let edge_0 = sub(&verts[tri[1] as usize].position, &verts[tri[0] as usize].position);
    let edge_1 = sub(&verts[tri[1] as usize].position, &verts[tri[2] as usize].position);
    cross(&edge_0, &edge_1)
}





struct VertIndices {
	indices: [usize; 3]
}

impl FromStr for VertIndices {
	type Err = Error;
	fn from_str(line: &str) -> Result<VertIndices, Error> {
		let mut vertex = [0, 0, 0];
		for i in line.split('/').map(|x| if x.is_empty() { Ok(0) } else { x.parse::<usize>() }).enumerate() {
			match i {
				(i, Ok(c)) => vertex[i] = c,
				_ => return Err(Error::from("Invalid vertex."))
			}
		}
		Ok(VertIndices { indices: vertex })
	}
}
