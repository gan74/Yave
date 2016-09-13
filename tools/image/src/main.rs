extern crate bincode;
extern crate rustc_serialize;

extern crate image;

use bincode::SizeLimit;
use bincode::rustc_serialize::*;

use std::env;
use std::fs::File;
use std::path::Path;

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

    let ref mut writer = BufWriter::new(File::create(file + ".rgba").unwrap());


    encode_into(&image.dimensions().0, writer, SizeLimit::Infinite);
    encode_into(&image.dimensions().1, writer, SizeLimit::Infinite);
    encode_into(&rgba.into_vec(), writer, SizeLimit::Infinite);

    println!("Done");
}
