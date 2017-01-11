use std::fs::{File};
use std::io::{BufReader, BufWriter, Write, Result};
use std::mem;
use std::slice;

mod types;
mod obj_loader;

use obj_loader::*;
use types::{Mesh};

fn main() {
	let mut args = std::env::args();
	args.next();

	let file_name = args.next().expect("Expected a file as input.");
	let mut file = File::open(&file_name).expect("Unable to open file.");

	let mesh = load_obj(&mut BufReader::new(&mut file)).expect("Unable read mesh.");
	
	let ref mut writer = BufWriter::new(File::create(file_name + ".ym").expect("Unable to create output file."));

	write_mesh(writer, &mesh).expect("Unable to write to output file.");
}



fn write_mesh<T: Write>(file: &mut T, mesh: &Mesh) -> Result<usize> {
    let version: u64 = 1;

    file.write(b"yave")
    	.and_then(|_| write_bin(file, &vec![version]))

        .and_then(|_| write_bin(file, &vec![mesh.vertices.len() as u32]))
    	.and_then(|_| write_bin(file, &mesh.vertices))

        .and_then(|_| write_bin(file, &vec![mesh.triangles.len() as u32]))
    	.and_then(|_| write_bin(file, &mesh.triangles))
}


fn write_bin<E, T: Write>(file: &mut T, v: &Vec<E>) -> Result<usize> {
    let slice_u8: &[u8] = unsafe {
        slice::from_raw_parts(
            v.as_ptr() as *const u8, 
            v.len() * mem::size_of::<E>()
        )
    };
    file.write(slice_u8)
}