
use std::io;

pub type Triangle = [u32; 3];
pub type Vec3 = [f32; 3];
pub type Vec2 = [f32; 2];

pub struct Vertex {
	pub position: Vec3,
	pub normal: Vec3,
	pub uv: Vec2
}

pub struct Mesh {
	pub vertices: Vec<Vertex>,
	pub triangles: Vec<Triangle>
}


#[derive(Debug)]
pub struct Error {
	msg: String
}

impl From<io::Error> for Error {
	fn from(err: io::Error) -> Self {
		let msg = format!("{}", err);
		Error{msg: msg}
	}
}

impl From<String> for Error {
	fn from(err: String) -> Self {
		Error{msg: err}
	}
}

impl From<&'static str> for Error {
	fn from(err: &str) -> Self {
		Error{msg: err.to_owned()}
	}
}


pub fn cross(a: &Vec3, b: &Vec3) -> Vec3 {
	[-(a[1] * b[2] - a[2] * b[1]), 
	 -(a[2] * b[0] - a[0] * b[2]), 
	 -(a[0] * b[1] - a[1] * b[0])]
}

pub fn sub(a: &Vec3, b: &Vec3) -> Vec3 {
	[a[0] - b[0], a[1] - b[1], a[2] - b[2]]
}

pub fn add(a: &Vec3, b: &Vec3) -> Vec3 {
	[a[0] +b[0], a[1] + b[1], a[2] + b[2]]
}

pub fn dot(a: &Vec3, b: &Vec3) -> f32 {
	a[0] * b[0] + a[1] * b[1] + a[2] * b[2]
}

pub fn normalize(vec: &Vec3) -> Vec3 {
	let len = dot(vec, vec).sqrt();
	[vec[0] / len, vec[1] / len, vec[2] / len]
}
