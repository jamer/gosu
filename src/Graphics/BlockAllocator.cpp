#include "BlockAllocator.hpp"
#include <stdexcept>
#include <vector>

struct Gosu::BlockAllocator::Impl
{
    unsigned width, height;
    unsigned availW, availH;

    typedef std::vector<unsigned> Used;
    typedef std::vector<Block> Blocks;
    Used used; // Height table.
    Blocks blocks;

    // Rebuild the height table where b was allocated.
    void recalcUsed(Block b)
    {
        std::fill_n(used.begin() + b.left, b.width, 0);
        for (Blocks::iterator i = blocks.begin(); i != blocks.end(); ++i)
        {
            if (i->left < b.left + b.width && b.left < i->left + i->width)
            {
                unsigned interLeft = std::max(i->left, b.left);
                unsigned interRight = std::min(i->left + i->width,
                    b.left + b.width);
                unsigned ibot = i->top + i->height;

                for (int i = interLeft; i < interRight; i++)
                    used[i] = std::max(used[i], ibot);
            }
        }
    }
};

Gosu::BlockAllocator::BlockAllocator(unsigned width, unsigned height)
: pimpl(new Impl)
{
    pimpl->width = width;
    pimpl->height = height;

    pimpl->availW = width;
    pimpl->availH = height;

    Impl::Used::iterator begin = pimpl->used.begin();
    pimpl->used.insert(begin, width, 0);
}

Gosu::BlockAllocator::~BlockAllocator()
{
}

unsigned Gosu::BlockAllocator::width() const
{
    return pimpl->width;
}

unsigned Gosu::BlockAllocator::height() const
{
    return pimpl->height;
}

bool Gosu::BlockAllocator::alloc(unsigned aWidth, unsigned aHeight, Block& b)
{
    unsigned x, i;
    unsigned bestH = height();
    unsigned tentativeH;

    if (aWidth > pimpl->availW && aHeight > pimpl->availH)
        return false;

    // Based on Id Software's block allocator from Quake 2, released under
    // the GPL.
    // See: http://fabiensanglard.net/quake2/quake2_opengl_renderer.php
    // Search for: LM_AllocBlock
    for (x = 0; x <= pimpl->width - aWidth; x++)
    {
        tentativeH = 0;
        for (i = 0; i < aWidth; i++)
        {
            if (pimpl->used[x+i] >= bestH)
                break;
            if (pimpl->used[x+i] > tentativeH)
                tentativeH = pimpl->used[x+i];
        }
        if (i == aWidth)
        {
            b.left = x;
            b.top = bestH = tentativeH;
            b.width = aWidth;
            b.height = aHeight;
        }
    }

    // There wasn't enough space for the bitmap.
    if (bestH + aHeight > pimpl->height) {
        // Remember this for later.
        if (aWidth > pimpl->availW && aHeight > pimpl->availH) {
            pimpl->availW = aWidth - 1;
            pimpl->availH = aHeight - 1;
        }
        return false;
    }

    // We've found a valid spot.
    for (i = 0; i < aWidth; i++)
        pimpl->used[b.left+i] = bestH + aHeight;
    pimpl->blocks.push_back(b);


    return true;
}

void Gosu::BlockAllocator::block(unsigned left, unsigned top, unsigned width, unsigned height)
{
    pimpl->blocks.push_back(Block(left, top, width, height));
}

void Gosu::BlockAllocator::free(unsigned left, unsigned top, unsigned width, unsigned height)
{
    for (Impl::Blocks::iterator i = pimpl->blocks.begin();
        i != pimpl->blocks.end(); ++i)
    {
        if (i->left == left && i->top == top && i->width == width && i->height == height)
        {
            Block b = *i;
            pimpl->blocks.erase(i);

            // Be optimistic again!
            pimpl->availW = pimpl->width;
            pimpl->availH = pimpl->height;

            // Look for freed-up space.
            pimpl->recalcUsed(b);
            return;
        }
    }

    throw std::logic_error("Tried to free an invalid block");
}
