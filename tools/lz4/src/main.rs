extern crate lz4;

use std::iter::FromIterator;
use std::env;
use std::fs::File;
use std::io::Result;
use std::io::Read;
use std::io::Write;
use std::path::Path;


fn main()
{
    println!("LZ4 version: {}", lz4::version());
    let suffix = ".lz4";
    for arg in Vec::from_iter(env::args())[1..].iter()
    {
        if arg.ends_with(suffix)
        {
            decompress(&Path::new(arg), &Path::new(&arg[0..arg.len()-suffix.len()])).unwrap();
        }
        else
        {
            compress(&Path::new(arg), &Path::new(&(arg.to_string() + suffix))).unwrap();
        }
    }
}

fn compress(src: &Path, dst: &Path) -> Result<()>
{
    println!("Compressing: {:?} -> {:?}", src, dst);
    let mut fi = try!(File::open(src));
    let mut fo = try!(lz4::EncoderBuilder::new().build(try!(File::create(dst))));
    try!(copy(&mut fi, &mut fo));
    match fo.finish() {
        (_, result) => result
    }
}

fn decompress(src: &Path, dst: &Path) -> Result<()>
{
    println!("Decompressing: {:?} -> {:?}", src, dst);
    let mut fi = try!(lz4::Decoder::new(try!(File::open(src))));
    let mut fo = try!(File::create(dst));
    copy(&mut fi, &mut fo)
}

fn copy(src: &mut Read, dst: &mut Write) -> Result<()>
{
    let mut buffer: [u8; 1024] = [0; 1024];
    loop
    {
        let len = try! (src.read(&mut buffer));
        if len == 0
        {
            break;
        }
        try!(dst.write_all(&buffer[0..len]));
    }
    Ok(())
}
