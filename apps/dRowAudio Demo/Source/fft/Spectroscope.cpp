/*
  ==============================================================================

    Spectroscope.cpp
    Created: 19 Oct 2010 11:03:01pm
    Author:  David Rowland

  ==============================================================================
*/

#include "Spectroscope.h"

Spectroscope::Spectroscope (int fftSizeLog2)
:	fftEngine       (fftSizeLog2),
	needsRepaint    (true),
	tempBlock       (fftEngine.getFFTSize()),
	circularBuffer  (fftEngine.getMagnitudesBuffer().getSize() * 4),
	logFrequency    (false)
{
	setOpaque (true);

	fftEngine.setWindowType (Window::Hann);
	numBins = fftEngine.getFFTProperties().fftSizeHalved;
    
    circularBuffer.reset();
    
    scopeImage = Image (Image::RGB,
                        100, 100,
                        false);
    scopeImage.clear (scopeImage.getBounds(), Colours::black);
}

Spectroscope::~Spectroscope()
{
}

void Spectroscope::resized()
{
    scopeImage = scopeImage.rescaled (jmax (1, getWidth()), jmax (1, getHeight()));
}

void Spectroscope::paint(Graphics &g)
{
    g.drawImageAt (scopeImage, 0, 0, false);
}

//============================================	
void Spectroscope::copySamples (const float* samples, int numSamples)
{
	circularBuffer.writeSamples (samples, numSamples);
	needToProcess = true;
}

void Spectroscope::renderScopeImage()
{
    if (needsRepaint)
	{
        Graphics g (scopeImage);

		const int w = getWidth();
		const int h = getHeight();
        
		g.setColour (Colours::black);
		g.fillAll();
        
		g.setColour (Colours::white);
		
        const int numBins = fftEngine.getMagnitudesBuffer().getSize() - 1;
        const float xScale = (float)w / (numBins + 1);
        const float* data = fftEngine.getMagnitudesBuffer().getData();
        

        float y2, y1 = jlimit (0.0f, 1.0f, float (1 + (toDecibels (data[0]) / 100.0f)));
        float x2, x1 = 0;
        
        if (logFrequency)
		{
			for (int i = 1; i < numBins; ++i)
			{
				y2 = jlimit (0.0f, 1.0f, float (1 + (toDecibels (data[i]) / 100.0f)));
				x2 = log10 (i + 1) * xScale;
                
				g.drawLine (x1, h-h*y1,
						    x2, h-h*y2);
				
				y1 = y2;
				x1 = x2;
			}	
		}
		else
		{
			for (int i = 0; i < numBins; ++i)
			{
				y2 = jlimit (0.0f, 1.0f, float (1 + (toDecibels (data[i]) / 100.0f)));
				x2 = (i + 1) * xScale;
				
				g.drawLine (x1, h - h * y1,
						    x2, h - h * y2);
				
				y1 = y2;
				x1 = x2;
			}	
		}
		
		needsRepaint = false;

        repaint();
	}
}

void Spectroscope::timerCallback()
{
	const int magnitudeBufferSize = fftEngine.getMagnitudesBuffer().getSize();
	float* magnitudeBuffer = fftEngine.getMagnitudesBuffer().getData();

    renderScopeImage();

	// fall levels here
	for (int i = 0; i < magnitudeBufferSize; i++)
		magnitudeBuffer[i] *= 0.707f;
}

void Spectroscope::process()
{
    jassert (circularBuffer.getNumFree() != 0); // buffer is too small!
    
    while (circularBuffer.getNumAvailable() > fftEngine.getFFTSize())
	{
		circularBuffer.readSamples (tempBlock.getData(), fftEngine.getFFTSize());
		fftEngine.performFFT (tempBlock);
		fftEngine.updateMagnitudesIfBigger();
		
		needsRepaint = true;
	}
}


