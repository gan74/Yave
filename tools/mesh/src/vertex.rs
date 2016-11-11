use std::ops::*;

#[derive(Copy, Clone, Debug, PartialEq, RustcEncodable, RustcDecodable)]
pub struct Vec3(pub f32, pub f32, pub f32);

pub type Position = Vec3;
pub type Normal = Vec3;
pub type Edge = Vec3;

#[derive(Copy, Clone, Debug, PartialEq, RustcEncodable, RustcDecodable)]
pub struct Uv(pub f32, pub f32);


#[derive(Copy, Clone, Debug, PartialEq, RustcEncodable, RustcDecodable)]
pub struct Vertex {
    pub position: Position,
    pub normal: Normal,
    pub uv: Uv
}

impl Vec3 {
    pub fn cross(self, rhs: Edge) -> Vec3 {
        let mut n = Vec3::default();
        n.0 = -(self.1 * rhs.2 - self.2 * rhs.1);
        n.1 = -(self.2 * rhs.0 - self.0 * rhs.2);
        n.2 = -(self.0 * rhs.1 - self.1 * rhs.0);
        n
    }

    pub fn dot(&self, rhs: &Vec3) -> f32 {
        self.0 * rhs.0 + self.1 * rhs.1 + self.2 * rhs.2
    }

    pub fn length2(&self) -> f32 {
        self.dot(self)
    }

    pub fn length(&self) -> f32 {
        self.length2().sqrt()
    }

    pub fn normalize(&mut self) {
        let mut len = self.length();
        if len < 0.000001 {
            len = 1.0;
        }
        self.0 /= len;
        self.1 /= len;
        self.2 /= len;
    }

    pub fn normalized(&self) -> Vec3 {
        let mut v = Vec3(self.0, self.1, self.2);
        v.normalize();
        v
    }

    fn get(&self, i: usize) -> Option<&f32> {
        match i {
            0 => Some(&self.0),
            1 => Some(&self.1),
            2 => Some(&self.2),
            _=> None
        }
    }

    fn get_mut(&mut self, i: usize) -> Option<&mut f32> {
        match i {
            0 => Some(&mut self.0),
            1 => Some(&mut self.1),
            2 => Some(&mut self.2),
            _=> None
        }
    }
}

impl Sub for Vec3 {
    type Output = Vec3;

    fn sub(self, rhs: Self) -> Vec3 {
        Vec3(self.0 - rhs.0, self.1 - rhs.1, self.2 - rhs.2)
    }
}

impl Add for Vec3 {
    type Output = Vec3;

    fn add(self, rhs: Self) -> Vec3 {
        Vec3(self.0 + rhs.0, self.1 + rhs.1, self.2 + rhs.2)
    }
}

impl Mul for Vec3 {
    type Output = Vec3;

    fn mul(self, rhs: Self) -> Vec3 {
        Vec3(self.0 * rhs.0, self.1 * rhs.1, self.2 * rhs.2)
    }
}

impl Div for Vec3 {
    type Output = Vec3;

    fn div(self, rhs: Self) -> Vec3 {
        Vec3(self.0 / rhs.0, self.1 / rhs.1, self.2 / rhs.2)
    }
}

impl Mul<f32> for Vec3 {
    type Output = Vec3;

    fn mul(self, rhs: f32) -> Vec3 {
        Vec3(self.0 * rhs, self.1 * rhs, self.2 * rhs)
    }
}

impl Div<f32> for Vec3 {
    type Output = Vec3;

    fn div(self, rhs: f32) -> Vec3 {
        Vec3(self.0 / rhs, self.1 / rhs, self.2 / rhs)
    }
}

impl AddAssign for Vec3 {
    fn add_assign(&mut self, rhs: Self) {
        self.0 += rhs.0;
        self.1 += rhs.1;
        self.2 += rhs.2;
    }
}

impl SubAssign for Vec3 {
    fn sub_assign(&mut self, rhs: Self) {
        self.0 -= rhs.0;
        self.1 -= rhs.1;
        self.2 -= rhs.2;
    }
}

impl MulAssign for Vec3 {
    fn mul_assign(&mut self, rhs: Self) {
        self.0 *= rhs.0;
        self.1 *= rhs.1;
        self.2 *= rhs.2;
    }
}

impl DivAssign for Vec3 {
    fn div_assign(&mut self, rhs: Self) {
        self.0 /= rhs.0;
        self.1 /= rhs.1;
        self.2 /= rhs.2;
    }
}


impl MulAssign<f32> for Vec3 {
    fn mul_assign(&mut self, rhs: f32) {
        self.0 *= rhs;
        self.1 *= rhs;
        self.2 *= rhs;
    }
}

impl DivAssign<f32> for Vec3 {
    fn div_assign(&mut self, rhs: f32) {
        self.0 /= rhs;
        self.1 /= rhs;
        self.2 /= rhs;
    }
}


impl Index<usize> for Vec3 {
    type Output = f32;

    fn index(&self, i: usize) -> &f32 {
        self.get(i).unwrap()
    }
}

impl IndexMut<usize> for Vec3 {
    fn index_mut(&mut self, i: usize) -> &mut f32 {
        self.get_mut(i).unwrap()
    }
}

pub struct IntoIter {
    vec: Vec3,
    index: usize
}

impl Iterator for IntoIter {
    type Item = f32;
    fn next(&mut self) -> Option<Self::Item> {
        self.index += 1;
        self.vec.get(self.index - 1).cloned()
    }
}

impl IntoIterator for Vec3 {
    type Item = f32;
    type IntoIter = IntoIter;

    fn into_iter(self) -> Self::IntoIter {
        IntoIter {
            vec: self,
            index: 0
        }
    }
}

impl Default for Vec3 {
    fn default() -> Vec3 {
        Vec3(0.0, 0.0, 0.0)
    }
}

impl Default for Uv {
    fn default() -> Uv {
        Uv(0.5, 0.5)
    }
}
