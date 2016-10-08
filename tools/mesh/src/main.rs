extern crate bincode;
extern crate rustc_serialize;

mod mesh;
mod vertex;
mod loader;
mod writer;
mod obj_loader;
mod yave_loader;

use mesh::*;
use vertex::*;
use loader::*;
use writer::*;
use yave_loader::*;
use obj_loader::*;

use std::time::{Duration, SystemTime};
use std::fs::File;
use std::io::{Write};
use std::io;


fn time<T, F: FnMut() -> T>(mut f: F, msg: String) -> T {
    let start = SystemTime::now();
    let r = f();
    println!("{} in {}ms", msg, to_millis(start.elapsed().unwrap()));
    r
}


struct Info {
    mesh: Mesh,
    name: String,
    saved: bool
}

impl Info {
    fn mut_mesh(&mut self) -> &mut Mesh {
        self.saved = false;
        &mut self.mesh
    }
}

fn to_millis(d: Duration) -> u64 {
    d.as_secs() * 1000 + (d.subsec_nanos() as u64) / 1_000_000
}

fn get_writters() -> Vec<Box<Writer>> {
    vec![Box::new(YaveWriter::new()), Box::new(BadObjWriter::new())]
}

fn get_loaders() -> Vec<Box<Loader>> {
    vec![Box::new(YaveLoader::new()), Box::new(ObjLoader::new())]
}

fn create_export_file(file_name: &str) -> Option<File> {
    match File::open(file_name) {
        Ok(_) => {
            if read_bool(&format!("{:?} already exists, do you want to overwrite it?", file_name)) {
                Some(File::create(file_name).expect("Unable to create output file"))
            } else {
                None
            }
        }
        _ => Some(File::create(file_name).expect("Unable to create output file"))
    }
}

fn export(info: &mut Info, file_name: &str) {
    for writer in get_writters() {
        if writer.is_supported(file_name) {
            match create_export_file(file_name) {
                Some(mut file) => {
                    time(|| writer.write(&info.mesh, &mut file).unwrap(), format!("{:?} exported", file_name));
                    info.saved = true;
                },
                None => println!("Unable to export mesh")
            }
            return
        }
    }
    println!("Unknown file format");
}

fn load(file_name: &str) -> Info {
    for loader in get_loaders() {
        if loader.is_supported(file_name) {
            return Info {
                mesh: time(|| loader.load(&mut File::open(file_name).expect("Unable to open file")).unwrap(), format!("{:?} loaded", file_name)),
                name: file_name.to_string(),
                saved: true
            }
        }
    }
    panic!("Unknown file format");
}

fn try_export<'a, T: Iterator<Item = &'a str>>(info: &mut Info, cmds: T) {
    let cmds = cmds.collect::<Vec<_>>();
    match cmds.len() {
        1 => export(info, cmds[0]),
        _ => println!("Wrong number of arguments for \"export\"")
    }
}

fn print_info<'a, T: Iterator<Item = &'a str>>(info: &mut Info, mut cmds: T) {
    if cmds.next().is_some() {
        println!("Too many arguments for \"info\"");
    }
    println!("{}{}", info.name, if info.saved { "" } else { "*" });
    println!("{} vertices, {} triangles", info.mesh.vertices.len(), info.mesh.indices.len());
}

fn quit(info: &Info) {
    if !info.saved {
        if !read_bool("You have unsaved work, are you sure?") {
            return
        }
    }
    std::process::exit(0)
}

fn scale<'a, T: Iterator<Item = &'a str>>(info: &mut Info, cmds: T) {
    let cmds = cmds.collect::<Vec<_>>();
    if cmds.len() != 3 {
        println!("Wrong number of arguments for \"scale\"")
    }
    let sc = cmds.iter().map(|x| x.parse::<f32>().unwrap()).collect::<Vec<_>>();
    info.mesh.scale(Vec3(sc[0], sc[1], sc[2]));
    info.saved = false;
}

fn read_line() -> String {
    io::stdout().flush().unwrap();
    let mut input = String::new();
    io::stdin().read_line(&mut input).unwrap();
    input
}

fn read_bool(q: &str) -> bool {
    print!("{} [y/n] ", q);
    let res = match read_line().trim().to_lowercase().as_str() {
        "y" | "yes" | "1" => true,
        _ =>  false
    };
    print!("\r\r\r");
    res
}

fn process_one<'a, T: Iterator<Item = &'a str>>(info: &mut Info, mut cmds: T) {
    if let Some(cmd) = cmds.next() {
        match cmd {
            "export" => try_export(info, cmds),
            "info" => print_info(info, cmds),
            "reverse_faces" => info.mut_mesh().reverse_faces(),
            "scale" | "s" => scale(info, cmds),
            "quit" | "q" => quit(info),
            _ => println!("Unknown command {:?}", cmd)
        }
    }
}

fn main() {
    let mut args = std::env::args();
    args.next();

    let mut info = load(&args.next().unwrap());

    if args.next().is_some() {
        panic!("Too many arguments");
    }

    loop {
        print!(">> ");
        process_one(&mut info, read_line().split_whitespace());
    }
}
