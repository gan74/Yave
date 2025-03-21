
#ifdef IMGUI_DEFINE_MATH_OPERATORS
static inline ImVec4  operator*(const ImVec4& lhs, const float rhs)     { return ImVec4(lhs.x * rhs, lhs.y * rhs, lhs.z * rhs, lhs.w * rhs); }
static inline ImVec4  operator/(const ImVec4& lhs, const float rhs)     { return ImVec4(lhs.x / rhs, lhs.y / rhs, lhs.z / rhs, lhs.w / rhs); }
static inline ImVec4  operator/(const ImVec4& lhs, const ImVec4& rhs)   { return ImVec4(lhs.x / rhs.x, lhs.y / rhs.y, lhs.z / rhs.z, lhs.w / rhs.w); }
static inline ImVec4  operator-(const ImVec4& lhs)                      { return ImVec4(-lhs.x, -lhs.y, -lhs.z, -lhs.w); }
#endif
