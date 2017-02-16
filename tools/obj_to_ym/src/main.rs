use std::fs::{File};
use std::io::{BufReader, BufWriter, Write, Result};
use std::mem;
use std::env;
use std::slice;

mod types;
mod obj_loader;

use obj_loader::*;
use types::{Mesh};

fn main() {
    for arg in env::args().skip(1) {
        process_file(arg)
    }
}

fn process_file(file_name: String) {
    let mut file = File::open(&file_name).expect("Unable to open file.");
    let mesh = load_obj(&mut BufReader::new(&mut file)).expect("Unable read mesh.");
    let ref mut writer = BufWriter::new(File::create(file_name + ".ym").expect("Unable to create output file."));
    write_mesh(writer, &mesh).expect("Unable to write to output file.");
}

fn write_mesh<T: Write>(file: &mut T, mesh: &Mesh) -> Result<usize> {
    let mesh_type: u32 = 1;
    let version: u32 = 2;

    file.write(b"yave")
        .and_then(|_| write_bin(file, &vec![mesh_type, version]))

        .and_then(|_| write_bin(file, &vec![mesh.radius()]))
		
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