/*use std::io::{BufRead};
use std::borrow::{Cow};
use std::mem;

use fbx_direct::reader::{FbxEvent, EventReader};
use fbx_direct::common::{OwnedProperty};

use types::*;


pub fn load_fbx(file: &mut BufRead) -> Result<Mesh, Error> {
	//dump_fbx(file);
	
	let mut positions = Vec::new();
	let mut normals = Vec::new();
	let mut uvs = Vec::new();
	let mut triangles = Vec::new();

	let parser = EventReader::new(file);
	for e in parser {
		match e {
			Ok(FbxEvent::StartNode { name, properties }) => {
				match name.as_ref() {
					"Vertices" => positions = parse_positions(properties.first().unwrap())?,
					"PolygonVertexIndex" => triangles = parse_triangles(properties.first().unwrap())?,
					"Normals" => println!("NORMALS"),
					"UV" => println!("UVS"),
					_ => ()
				}
			},
			Err(_) => return Err(Error::from("Unknown FBX error.")),
			_ => ()
		}
	};

	Ok(Mesh {
		vertices: vertices,
		triangles: triangles
	})
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
		if tri_data.len() % 3 != 0 {
			Err(Error::from("Invalid triangle data"))
		} else {
			let tri_vec = tri_data.to_vec();
			unsafe {
				let len = tri_vec.len() / 3;
				let mut triangles = mem::transmute::<Vec<i32>, Vec<Triangle>>(tri_vec);
				triangles.set_len(len);
				Ok(triangles)
			}
		}
	} else {
		Err(Error::from("No triangles data."))
	}
}

pub fn dump_fbx(file: &mut BufRead) {
	fn indent(size: usize) -> String {
		const INDENT: &'static str = "	";
		(0..size).map(|_| INDENT)
			.fold(String::with_capacity(size * INDENT.len()), |r, s| r + s)
	}

	let mut depth = 0;
	let parser = EventReader::new(file);
	for e in parser {
		match e {
			Ok(FbxEvent::StartNode { name, .. }) => {
				println!("{}{:?}", indent(depth), name);
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
}*/
