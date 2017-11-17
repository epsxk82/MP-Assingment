/*소프트웨어 겹섭형 2배 다운스케일링을 수행*/
__kernel void SWBilinearDownScaling(__read_only image2d_t sourceImage, __write_only image2d_t destinationImage)
{ 
	const sampler_t sampler = CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP | CLK_FILTER_NEAREST;
	int2 colorPosition = (int2)(get_global_id(0), get_global_id(1));

	/*원본 이미지의 대상 좌표들을 가져옴*/
	int2 leftTop = colorPosition * (int2)(2, 2);
	int2 rightTop = leftTop + (int2)(1, 0);
	int2 leftBottom = leftTop + (int2)(0, 1);
	int2 rightBottom = leftTop + (int2)(1, 1);
	
	/*원본 이미지의 대상 픽셀들을 가져옴*/
	float4 leftTopColor = read_imagef(sourceImage, sampler, leftTop);
	float4 rightTopColor = read_imagef(sourceImage, sampler, rightTop);
	float4 leftBottomColor = read_imagef(sourceImage, sampler, leftBottom);
	float4 rightBottomColor = read_imagef(sourceImage, sampler, rightBottom);

	/*2배 수행이므로 4개의 가중치는 동일*/
	float4 quater = (float4)(0.25, 0.25, 0.25, 0.25);
	/*보간된 컬러 계산*/
	float4 color = leftTopColor * quater  + rightTopColor * quater + leftBottomColor * quater + rightBottomColor * quater;
	write_imagef(destinationImage, colorPosition, color);
}