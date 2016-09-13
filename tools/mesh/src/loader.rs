use std::io::Read;
use std::io;

use mesh::*;

pub trait Loader {
    fn extentions(&self) -> Vec<&'static str>;

    fn is_supported(&self, name: &str) -> bool {
        self.extentions().iter().any(|ext| name.ends_with(ext))
    }

    fn load(&self, file: &mut Read) -> io::Result<Mesh>;
}
