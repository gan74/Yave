use std::path::{Path};
use std::fs::{File};
use std::io::{BufReader, BufRead};
use std::borrow::{Cow};
use std::mem;

use fbx_direct::reader::{FbxEvent, EventReader};
use fbx_direct::common::{OwnedProperty};

use types::*;


pub fn load_fbx(path: &Path) -> Result<Mesh, Error> {
	//dump_fbx(path);
	
	/*let file = BufReader::new(File::open(path).expect("Unable to open file."));

	let mut positions = Vec::new();
	let mut normals = Vec::new();
	//let mut uvs = Vec::new();
	let mut triangles = Vec::new();

	let parser = EventReader::new(file);
	for e in parser {
		match e {
			Ok(FbxEvent::StartNode { name, properties }) => {
				match name.as_ref() {
					"Vertices" => positions = parse_positions(properties.first().unwrap())?,
					"PolygonVertexIndex" => triangles = parse_triangles(properties.first().unwrap())?,
					"Normals" => normals = parse_positions(properties.first().unwrap())?,
					"UV" => println!("UVS"),
					_ => ()
				}
			},
			Err(_) => return Err(Error::from("Unknown FBX error.")),
			_ => ()
		}
	}
	println!("Loaded FBX: {} vertices, {} normals, {} triangles", positions.len(), normals.len(), triangles.len());
	Ok(Mesh {
		vertices: positions.iter().zip(normals.iter()).map(|p| Vertex { position: *p.0, normal: *p.1, uv: [0.0; 2] } ).collect(),
		triangles: triangles
	})*/
	Err(Error::from("FBX not supported."))
}

fn parse_positions(prop: &OwnedProperty) -> Result<Vec<Vec3>, Error> {
	if let Some(pos_data) = prop.get_vec_f32() {
		if pos_data.len() % 3 != 0 {
			Err(Error::from("Invalid position data"))
		} else {
			let pos_vec = pos_data.to_vec();
			unsafe {
				let len = pos_vec.len() / 3;
				let mut positions = mem::transmute::<Vec<f32>, Vec<Vec3>>(pos_vec);
				positions.set_len(len);
				Ok(positions)
			}
		}
	} else {
		Err(Error::from("No position data."))
	}
}

fn parse_triangles(prop: &OwnedProperty) -> Result<Vec<Triangle>, Error> {
	if let Some(tri_data) = prop.get_vec_i32() {
		Ok(triangulate(tri_data.to_vec()))
	} else {
		Err(Error::from("No triangles data."))
	}
}

fn triangulate(tris: Vec<i32>) -> Vec<Triangle> {
	fn build_tris(tris: &[i32], triangles: &mut Vec<Triangle>) {
		let len = tris.len();
		let last = (tris[len - 1] ^ -1) as u32;
		if len >= 3 {
			for i in 0..len - 2 {
				triangles.push([tris[i] as u32, tris[i + 1] as u32, last]);
			}
		} else {
			println!("{:?}", tris.len());
		}
	};

	let mut start = 0;
	let mut triangles = Vec::new();
	for tri in tris.iter().enumerate() {
		if *tri.1 < 0 {
			let end = tri.0 + 1;
			build_tris(&tris[start..end], &mut triangles);
			start = end;
		}
	}
	triangles
}

pub fn dump_fbx(path: &Path) {
	let file = BufReader::new(File::open(path).expect("Unable to open file."));
	fn indent(size: usize) -> String {
		const INDENT: &'static str = "	";
		(0..size).map(|_| INDENT)
			.fold(String::with_capacity(size * INDENT.len()), |r, s| r + s)
	}

	let mut depth = 0;
	let parser = EventReader::new(file);
	for e in parser {
		match e {
			Ok(FbxEvent::StartNode { name, properties }) => {
				let mut prop_str = format!("{:?}", properties);
				prop_str.truncate(64);
				println!("{}{}: {}", indent(depth), name, prop_str);
				depth += 1;
			},
			Ok(FbxEvent::EndNode) => depth -= 1,
			Ok(_) => (),
			Err(e) => {
				println!("Error: {:?}", e); 
				return;
			}
		}
	};
}
