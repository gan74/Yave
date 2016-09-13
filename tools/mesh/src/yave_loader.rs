use std::io::{BufReader, Read, Error, ErrorKind};
use std::io;

use bincode::SizeLimit;
use bincode::rustc_serialize::*;

use loader::*;
use mesh::*;
use vertex::*;

pub struct YaveLoader {
}

impl YaveLoader {
    pub fn new() -> YaveLoader {
        YaveLoader{}
    }
}

impl Loader for YaveLoader {
    fn extentions(&self) -> Vec<&'static str> {
        vec![".ym"]
    }

    fn load(&self, file: &mut Read) -> io::Result<Mesh> {
        fn buf_helper<T: Read>(mut file: T) -> io::Result<Mesh> {
            if decode_from::<T, u32>(&mut file, SizeLimit::Infinite).unwrap() != 0x79617665 {
                return Err(Error::new(ErrorKind::InvalidData, "invalid magic number"));
            }
            if decode_from::<T, u64>(&mut file, SizeLimit::Infinite).unwrap() != 1 {
                return Err(Error::new(ErrorKind::InvalidData, "invalid file version"));
            }

            let vertices = decode_from::<T, Vec<Vertex>>(&mut file, SizeLimit::Infinite).unwrap();
            let indices = decode_from::<T, Vec<(u32, u32, u32)>>(&mut file, SizeLimit::Infinite).unwrap();

            Ok(Mesh {
                vertices: vertices,
                indices: indices
            })
        }
        buf_helper(BufReader::new(file))
    }
}
