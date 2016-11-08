use std::io::{BufWriter, Write, Error, ErrorKind};
use std::io;

use mesh::*;

use bincode::*;
use bincode::rustc_serialize::*;
//use rustc_serialize::*;


pub trait Writer {
    fn extentions(&self) -> Vec<&'static str>;

    fn is_supported(&self, name: &str) -> bool {
        self.extentions().iter().any(|ext| name.ends_with(ext))
    }

    fn write(&self, mesh: &Mesh, file: &mut Write) -> io::Result<()>;
}

pub struct YaveWriter {
}

impl YaveWriter {
    pub fn new() -> YaveWriter {
        YaveWriter {}
    }
}

impl Writer for YaveWriter {
    fn write(&self, mesh: &Mesh, file: &mut Write) -> io::Result<()> {
        fn buf_helper<T: Write>(mesh: &Mesh, file: &mut T) -> Result<(), EncodingError> {
            let magic = 0x79617665 as u32;
            let version: u64 = 1;

            try!(encode_into(&magic, file, SizeLimit::Infinite));
            try!(encode_into(&version, file, SizeLimit::Infinite));

            try!(encode_into(&mesh.vertices, file, SizeLimit::Infinite));
            try!(encode_into(&mesh.indices, file, SizeLimit::Infinite));

            Ok(())
        }
        match buf_helper(mesh, &mut BufWriter::new(file)) {
            Ok(m) => Ok(m),
            Err(err) => Err(Error::new(ErrorKind::InvalidData, err))
        }
    }

    fn extentions(&self) -> Vec<&'static str> {
        vec![".ym"]
    }
}



pub struct BadObjWriter {
}

impl BadObjWriter {
    pub fn new() -> BadObjWriter {
        BadObjWriter {}
    }
}

impl Writer for BadObjWriter {
    fn write(&self, mesh: &Mesh, file: &mut Write) -> io::Result<()> {
        let mut buf = BufWriter::new(file);
        for v in &mesh.vertices {
            let pos = format!("v {} {} {}\n", v.position.0, v.position.1, v.position.2);
            try!(buf.write(pos.into_bytes().as_slice()));

            let uv = format!("vt {} {}\n", v.uv.0, v.uv.1);
            try!(buf.write(uv.into_bytes().as_slice()));

            let norm = format!("vn {} {} {}\n", v.normal.0, v.normal.1, v.normal.2);
            try!(buf.write(norm.into_bytes().as_slice()));
        }

        try!(buf.write(b"s off\n"));

        for tri in &mesh.indices {
            let line = format!("f {}/{}/{} {}/{}/{} {}/{}/{}\n",
                            tri.0 + 1, tri.0 + 1, tri.0 + 1,
                            tri.1 + 1, tri.1 + 1, tri.1 + 1,
                            tri.2 + 1, tri.2 + 1, tri.2 + 1,
                        );
            try!(buf.write(line.into_bytes().as_slice()));
        }

        Ok(())
    }

    fn extentions(&self) -> Vec<&'static str> {
        vec![".obj"]
    }
}
