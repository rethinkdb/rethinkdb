# -*- coding: utf-8 -*-
from __future__ import unicode_literals

from Quartz import *
import math

_REMOVABLE_DISK_PATH = '/System/Library/Extensions/IOStorageFamily.kext/Contents/Resources/Removable.icns'

def badge_disk_icon(badge_file, output_file):
    # Load the Removable disk icon
    url = CFURLCreateWithFileSystemPath(None, _REMOVABLE_DISK_PATH,
                                        kCFURLPOSIXPathStyle, False)
    backdrop = CGImageSourceCreateWithURL(url, None)
    backdropCount = CGImageSourceGetCount(backdrop)
    
    # Load the badge
    url = CFURLCreateWithFileSystemPath(None, badge_file,
                                        kCFURLPOSIXPathStyle, False)
    badge = CGImageSourceCreateWithURL(url, None)
    assert badge is not None, 'Unable to process image file: %s' % badge_file
    badgeCount = CGImageSourceGetCount(badge)
    
    # Set up a destination for our target
    url = CFURLCreateWithFileSystemPath(None, output_file,
                                        kCFURLPOSIXPathStyle, False)
    target = CGImageDestinationCreateWithURL(url, 'com.apple.icns',
                                             backdropCount, None)

    # Get the RGB colorspace
    rgbColorSpace = CGColorSpaceCreateWithName(kCGColorSpaceGenericRGB)

    # Scale
    scale = 1.0
    
    # Perspective transform
    corners = ((0.2, 0.95), (0.8, 0.95), (0.85, 0.35), (0.15, 0.35))

    # Translation
    position = (0.5, 0.5)

    for n in range(backdropCount):
        props = CGImageSourceCopyPropertiesAtIndex(backdrop, n, None)
        width = props['PixelWidth']
        height = props['PixelHeight']
        dpi = props['DPIWidth']
    
        # Choose the best sized badge image
        bestWidth = None
        bestHeight = None
        bestBadge = None
        bestDPI = None
        for m in range(badgeCount):
            badgeProps = CGImageSourceCopyPropertiesAtIndex(badge, m, None)
            badgeWidth = badgeProps['PixelWidth']
            badgeHeight = badgeProps['PixelHeight']
            badgeDPI = badgeProps['DPIWidth']
        
            if bestBadge is None or (badgeWidth <= width
                                    and (bestWidth > width
                                        or badgeWidth > bestWidth
                                        or (badgeWidth == bestWidth
                                            and badgeDPI == dpi))):
                bestBadge = m
                bestWidth = badgeWidth
                bestHeight = badgeHeight
                bestDPI = badgeDPI

        badgeImage = CGImageSourceCreateImageAtIndex(badge, bestBadge, None)
        badgeCI = CIImage.imageWithCGImage_(badgeImage)
    
        backgroundImage = CGImageSourceCreateImageAtIndex(backdrop, n, None)
        backgroundCI = CIImage.imageWithCGImage_(backgroundImage)
    
        compositor = CIFilter.filterWithName_('CISourceOverCompositing')
        lanczos = CIFilter.filterWithName_('CILanczosScaleTransform')
        perspective = CIFilter.filterWithName_('CIPerspectiveTransform')
        transform = CIFilter.filterWithName_('CIAffineTransform')
    
        lanczos.setValue_forKey_(badgeCI, kCIInputImageKey)
        lanczos.setValue_forKey_(scale * float(width)/bestWidth, kCIInputScaleKey)
        lanczos.setValue_forKey_(1.0, kCIInputAspectRatioKey)

        topLeft = (width * scale * corners[0][0],
                   width * scale * corners[0][1])
        topRight = (width * scale * corners[1][0],
                    width * scale * corners[1][1])
        bottomRight = (width * scale * corners[2][0],
                       width * scale * corners[2][1])
        bottomLeft = (width * scale * corners[3][0],
                      width * scale * corners[3][1])

        out = lanczos.valueForKey_(kCIOutputImageKey)
        if width >= 16:
            perspective.setValue_forKey_(out, kCIInputImageKey)
            perspective.setValue_forKey_(CIVector.vectorWithX_Y_(*topLeft),
                                         'inputTopLeft')
            perspective.setValue_forKey_(CIVector.vectorWithX_Y_(*topRight),
                                         'inputTopRight')
            perspective.setValue_forKey_(CIVector.vectorWithX_Y_(*bottomRight),
                                         'inputBottomRight')
            perspective.setValue_forKey_(CIVector.vectorWithX_Y_(*bottomLeft),
                                         'inputBottomLeft')
            out = perspective.valueForKey_(kCIOutputImageKey)

        tfm = NSAffineTransform.transform()
        tfm.translateXBy_yBy_(math.floor((position[0] - 0.5 * scale) * width),
                              math.floor((position[1] - 0.5 * scale) * height))

        transform.setValue_forKey_(out, kCIInputImageKey)
        transform.setValue_forKey_(tfm, 'inputTransform')
        out = transform.valueForKey_(kCIOutputImageKey)
    
        compositor.setValue_forKey_(out, kCIInputImageKey)
        compositor.setValue_forKey_(backgroundCI, kCIInputBackgroundImageKey)

        result = compositor.valueForKey_(kCIOutputImageKey)

        cgContext = CGBitmapContextCreate(None,
                                        width,
                                        height,
                                        8,
                                        0,
                                        rgbColorSpace,
                                        kCGImageAlphaPremultipliedLast)
        context = CIContext.contextWithCGContext_options_(cgContext, None)

        context.drawImage_inRect_fromRect_(result,
                                           ((0, 0), (width, height)),
                                           ((0, 0), (width, height)))

        image = CGBitmapContextCreateImage(cgContext)

        CGImageDestinationAddImage(target, image, props)

    CGImageDestinationFinalize(target)
    
