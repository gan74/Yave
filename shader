const float f0 = 0.54;

float sqr(float x) { 
	return x * x; 
}

float GGX(float alpha, float cosT) {
	float cos2 = sqr(cosT);
	float tan2 = (1.0 - cos2) / cos2;
	return 1.0 / PI * sqr(alpha / (cos2 * (sqr(alpha) + tan2)));
}

float G1(float alpha, float cosT) {
	//return cosT / (cosT * (1.0 - alpha) + alpha);
	float g = cosT + sqrt(1.0 + alpha * (1.0 - sqr(cosT)));
	return 2.0 * cosT / g;
}

float F(float f0, float cosT) {
	return f0 + (1.0 - f0) * (pow(1.0 - cosT, 5.0));
}


vec3 s_BRDF(vec3 C, vec3 L, vec3 V, vec3 N) {
	vec3 H = normalize(L + V);
	float alpha = max(sqr(n_Roughness), 0.001);
	float NdotL = dot(L, N);
	float NdotV = dot(N, V);
	float d = max(0.0, 4.0 * NdotV * NdotL) + 0.001; 
	float D = GGX(alpha, dot(N, H));
	float G = G1(alpha, NdotL) * G1(alpha, NdotV);
	vec3 F = mix(vec3(1.0), C, n_Metallic) /* F(f0, dot(L, H))*/;
	return max(vec3(0.0), D * G * F / d);
}

vec3 d_BRDF(vec3 C, vec3 L, vec3 V, vec3 N) {
	return max(vec3(0.0), C * (1.0 - F(f0, dot(normalize(L + V), V))) * dot(N, L));
	/*float R2 = sqr(n_Roughness);
	float A = 1.0 - 0.5 * R2 / (R2 + 0.33);
	float B = 0.45 * R2 / (R2 + 0.09);
	float LdotV = dot(L, V);
	float NdotL = dot(L, N);
	float NdotV = dot(N, V);
	float d = max(0.0, dot(normalize(V - N * NdotV), normalize(L - N * NdotL)));
	
	float avn = acos(NdotV);
	float aln = acos(NdotL);
	return max(vec3(0.0), C * (A + B * d * sin(max(avn, aln)) * tan(min(avn, aln)))) * (1.0 - n_Metallic);*/
}