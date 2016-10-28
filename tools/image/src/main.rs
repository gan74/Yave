extern crate bincode;
extern crate rustc_serialize;
extern crate lz4;

extern crate image;

use bincode::SizeLimit;
use bincode::rustc_serialize::*;

use std::env;
use std::fs::File;
use std::path::Path;

use std::io::Result;
use std::io::Read;
use std::io::Write;

use std::io::BufWriter;

use image::*;


fn main() {
    let file = if env:: args().count() == 2 {
        env::args().nth(1).unwrap()
    } else {
        panic!("Invalid number of argument")
    };

    let image = image::open(&Path::new(&file)).unwrap();

    println!("Image loaded");

    let rgba = image.flipv().to_rgba();

    let ref mut writer = BufWriter::new(File::create(file.clone() + ".rgba").unwrap());


    encode_into(&image.dimensions().0, writer, SizeLimit::Infinite);
    encode_into(&image.dimensions().1, writer, SizeLimit::Infinite);
    encode_into(&rgba.into_vec(), writer, SizeLimit::Infinite);

    compress(file + ".rgba").unwrap();

    println!("Done");
}

fn compress(src: String) -> Result<()> {
    let mut fi = try!(File::open(&src));
    let mut fo = try!(lz4::EncoderBuilder::new().build(try!(File::create(src + ".lz4"))));
    try!(copy(&mut fi, &mut fo));
    match fo.finish() {
        (_, result) => result
    }
}

fn decompress(src: &String, dst: &String) -> Result<()> {
    let mut fi = try!(lz4::Decoder::new(try!(File::open(src))));
    let mut fo = try!(File::create(dst));
    copy(&mut fi, &mut fo)
}

fn copy(src: &mut Read, dst: &mut Write) -> Result<()> {
    let mut buffer: [u8; 1024] = [0; 1024];
    loop {
        let len = try! (src.read(&mut buffer));
        if len == 0 {
            break;
        }
        try!(dst.write_all(&buffer[0..len]));
    }
    Ok(())
}