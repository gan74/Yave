const float f0 = 0.04;

float sqr(float x) { 
	return x * x; 
}

vec3 lerp(vec3 X, vec3 Y, float t) {
	return X * (1.0 - t) + Y * t;
}

float GGX(float alpha, float NdotH) {
	float NdotH2 = sqr(NdotH);
	float D = NdotH2 * alpha + (1.0 - NdotH2);
	return alpha / (PI * sqr(D)) - 1.0 / PI;
}

float G1(float alpha, float VdotN) {
	float D = max(0.0, VdotN);
	return D / (D * (1.0 - alpha) + alpha);
}

float F(float f0, float cosT) {
	return f0 + (1.0 - f0) * (1.0 - pow(1.0 - cosT, 5.0));
}

float Beckmann(float NdotH, float alpha) {
	float NdotH2 = sqr(NdotH);
	float x = (NdotH2 - 1.0) / (alpha * NdotH2);
	return vec3(exp(x) / (PI * alpha * sqr(NdotH2)));
}

vec3 s_BRDF(vec3 C, vec3 L, vec3 V, vec3 N, vec3 X, vec3 Y) {
	vec3 H = normalize(L + V);
	float alpha = max(sqr(n_Roughness), 0.001);
	float NdotL = dot(L, N);
	float NdotV = dot(N, V);
	float d = max(0.0, 4.0 * NdotV * NdotL) + 0.001; 
	
	float D = GGX(alpha, dot(N, H));
	float G = G1(alpha, NdotL) * G1(alpha, NdotV);
	vec3 F = lerp(vec3(1.0), C, n_Metallic) /* F(f0, dot(L, H))*/;
	return vec3(D * G * F / d);
}


vec3 d_BRDF(vec3 C, vec3 L, vec3 V, vec3 N, vec3 X, vec3 Y) {
	float R2 = sqr(n_Roughness);
	float A = 1.0 - 0.5 * R2 / (R2 + 0.33);
	float B = 0.45 * R2 / (R2 + 0.09);
	float LdotV = dot(L, V);
	float NdotL = dot(L, N);
	float NdotV = dot(N, V);
	float s = LdotV - NdotL * NdotV;
	float t = mix(1.0, max(NdotL, NdotV), step(0.0, s));
	float d = max(0.0, dot(normalize(V - N * NdotV), normalize(L - N * NdotL)));
	return C * max(0.0, NdotL) * (A + B * s / t);
}