//
//  GraphView_MutationLossTimeHistogram.m
//  SLiM
//
//  Created by Ben Haller on 3/1/15.
//	A product of the Messer Lab, http://messerlab.org/software/
//

//	This file is part of SLiM.
//
//	SLiM is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by
//	the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
//
//	SLiM is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
//	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
//
//	You should have received a copy of the GNU General Public License along with SLiM.  If not, see <http://www.gnu.org/licenses/>.


#import "GraphView_MutationLossTimeHistogram.h"
#import "SLiMWindowController.h"


@implementation GraphView_MutationLossTimeHistogram

- (id)initWithFrame:(NSRect)frameRect withController:(SLiMWindowController *)controller
{
	if (self = [super initWithFrame:frameRect withController:controller])
	{
		[self setHistogramBinCount:10];
		
		[self setXAxisMax:100];
		[self setXAxisMajorTickInterval:20];
		[self setXAxisMinorTickInterval:10];
		[self setXAxisMajorTickModulus:2];
		[self setXAxisTickValuePrecision:0];
		
		[self setXAxisLabelString:@"Mutation loss time"];
		[self setYAxisLabelString:@"Proportion of lost mutations"];
		
		[self setShowHorizontalGridLines:YES];
	}
	
	return self;
}

- (void)dealloc
{
	[super dealloc];
}

- (double *)lossTimeDataWithController:(SLiMWindowController *)controller
{
	int binCount = [self histogramBinCount];
	int mutationTypeCount = (int)controller->sim->mutation_types_.size();
	uint32 *histogram = controller->sim->population_.mutationLossTimes;
	uint32 histogramBins = controller->sim->population_.mutationLossGenSlots;	// fewer than binCount * mutationTypeCount may exist
	static double *rebin = NULL;
	static uint32 rebinBins = 0;
	uint32 usedRebinBins = binCount * mutationTypeCount;
	
	// re-bin for display; use double, normalize, etc.
	if (!rebin || (rebinBins < usedRebinBins))
	{
		rebinBins = usedRebinBins;
		rebin = (double *)realloc(rebin, rebinBins * sizeof(double));
	}
	
	for (int i = 0; i < usedRebinBins; ++i)
		rebin[i] = 0.0;
	
	for (int i = 0; i < binCount; ++i)
	{
		for (int j = 0; j < mutationTypeCount; ++j)
		{
			int histIndex = j + i * mutationTypeCount;
			
			if (histIndex < histogramBins)
				rebin[histIndex] += histogram[histIndex];
		}
	}
	
	// normalize within each mutation type
	for (int mutationTypeIndex = 0; mutationTypeIndex < mutationTypeCount; ++mutationTypeIndex)
	{
		uint32 total = 0;
		
		for (int bin = 0; bin < binCount; ++bin)
		{
			int binIndex = mutationTypeIndex + bin * mutationTypeCount;
			
			total += rebin[binIndex];
		}
		
		for (int bin = 0; bin < binCount; ++bin)
		{
			int binIndex = mutationTypeIndex + bin * mutationTypeCount;
			
			rebin[binIndex] /= (double)total;
		}
	}
	
	return rebin;
}

- (void)drawGraphInInteriorRect:(NSRect)interiorRect withController:(SLiMWindowController *)controller
{
	double *plotData = [self lossTimeDataWithController:controller];
	int binCount = [self histogramBinCount];
	int mutationTypeCount = (int)controller->sim->mutation_types_.size();
	
	// plot our histogram bars
	[self drawGroupedBarplotInInteriorRect:interiorRect withController:controller buffer:plotData subBinCount:mutationTypeCount mainBinCount:binCount firstBinValue:0.0 mainBinWidth:10.0];
}

- (NSSize)legendSize
{
	return [self mutationTypeLegendSize];	// we use the prefab mutation type legend
}

- (void)drawLegendInRect:(NSRect)legendRect
{
	[self drawMutationTypeLegendInRect:legendRect];		// we use the prefab mutation type legend
}

- (NSString *)stringForDataWithController:(SLiMWindowController *)controller
{
	NSMutableString *string = [NSMutableString stringWithString:@"# Graph data: Mutation loss time histogram\n"];
	
	[string appendString:[self dateline]];
	[string appendString:@"\n\n"];
	
	double *plotData = [self lossTimeDataWithController:controller];
	int binCount = [self histogramBinCount];
	SLiMSim *sim = controller->sim;
	int mutationTypeCount = (int)sim->mutation_types_.size();
	auto mutationTypeIter = sim->mutation_types_.begin();
	
	for (int j = 0; j < mutationTypeCount; ++j, ++mutationTypeIter)
	{
		MutationType *mutationType = (*mutationTypeIter).second;
		
		[string appendFormat:@"\"m%d\", ", mutationType->mutation_type_id_];
		
		for (int i = 0; i < binCount; ++i)
		{
			int histIndex = j + i * mutationTypeCount;
			
			[string appendFormat:@"%.4f, ", plotData[histIndex]];
		}
		
		[string appendString:@"\n"];
	}
	
	// Get rid of extra commas
	[string replaceOccurrencesOfString:@", \n" withString:@"\n" options:0 range:NSMakeRange(0, [string length])];
	
	return string;
}

@end




























































