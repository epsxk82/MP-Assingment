/*소프트웨어 겹섭형 2배 업스케일링을 수행*/
__kernel void SWBilinearUpScaling(__read_only image2d_t sourceImage, __write_only image2d_t destinationImage)
{
	const sampler_t sampler = CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP | CLK_FILTER_NEAREST;
	int2 colorPosition = (int2)(get_global_id(0), get_global_id(1));

	/*원본 이미지의 대상 좌표들을 가져옴*/
	int2 leftTop = convert_int2(convert_float2(colorPosition) / (float2)(2.0, 2.0));
	int2 rightTop = leftTop + (int2)(1, 0);
	int2 leftBottom = leftTop + (int2)(0, 1);
	int2 rightBottom = leftTop + (int2)(1, 1);
	
	/*원본 이미지의 대상 픽셀들을 가져옴*/
	float4 leftTopColor = read_imagef(sourceImage, sampler, leftTop);
	float4 rightTopColor = read_imagef(sourceImage, sampler, rightTop);
	float4 leftBottomColor = read_imagef(sourceImage, sampler, leftBottom);
	float4 rightBottomColor = read_imagef(sourceImage, sampler, rightBottom);

	/*대상 이미지의 보간 좌표들을 계산*/
	int2 scaleFactorDimension = (int2)(2, 2);
	leftTop = leftTop * scaleFactorDimension;
	rightTop = rightTop * scaleFactorDimension;
	leftBottom = leftBottom * scaleFactorDimension;
	rightBottom = rightBottom * scaleFactorDimension;

	/*가중치를 계산하고 컬러를 보간*/
	float horizontalTopWeight = convert_float(colorPosition.x - leftTop.x) / convert_float(rightTop.x - leftTop.x);
	float invertHorizontalTopWeight = 1.0 - horizontalTopWeight;
	float4 horizontalTopLinearizedColor = leftTopColor * (float4)(invertHorizontalTopWeight, invertHorizontalTopWeight, invertHorizontalTopWeight, invertHorizontalTopWeight) 
		+ rightTopColor * (float4)(horizontalTopWeight, horizontalTopWeight, horizontalTopWeight, horizontalTopWeight);
	float horizontalBottomWeight = convert_float(colorPosition.x - leftBottom.x) / convert_float(rightBottom.x - leftBottom.x);
	float invertHorizontalBottomWeight = 1.0 - horizontalBottomWeight;
	float4 horizontalBottomLinearizedColor = leftBottomColor * (float4)(invertHorizontalBottomWeight, invertHorizontalBottomWeight, invertHorizontalBottomWeight, invertHorizontalBottomWeight) 
		+ rightBottomColor * (float4)(horizontalBottomWeight, horizontalBottomWeight, horizontalBottomWeight, horizontalBottomWeight);
	float verticalWeight = convert_float(colorPosition.y - leftTop.y) / convert_float(leftBottom.y - leftTop.y);
	float invertVerticalWeight = 1.0 - verticalWeight;
	float4 color = horizontalTopLinearizedColor * (float4)(invertVerticalWeight, invertVerticalWeight, invertVerticalWeight, invertVerticalWeight) 
		+ horizontalBottomLinearizedColor * (float4)(verticalWeight, verticalWeight, verticalWeight, verticalWeight);
	
	write_imagef(destinationImage, colorPosition, color);
}