//BL_COPYRIGHT_NOTICE

//
// $Id: MultiFab.cpp,v 1.17 1998-06-13 17:28:58 lijewski Exp $
//

#ifdef BL_USE_NEW_HFILES
#include <iostream>
#include <iomanip>
using std::cin;
using std::cout;
using std::cerr;
using std::endl;
using std::setw;
#else
#include <iostream.h>
#include <iomanip.h>
#endif

#include <Assert.H>
#include <Misc.H>
#include <MultiFab.H>
#include <ParallelDescriptor.H>
#include <Geometry.H>

#if defined(BL_ARCH_IEEE)
#ifdef BL_USE_DOUBLE
    const Real INFINITY = 1.0e100;
#elif  BL_USE_FLOAT
    const Real INFINITY = 1.0e35;
#endif
#elif defined(BL_ARCH_CRAY)
    const Real INFINITY = 1.0e100;
#endif

MultiFab::MultiFab ()
    :
    m_FB_mfcd(0),
    m_FB_fbid(0),
    m_FPB_mfcd(0)
{}

MultiFab::MultiFab (const BoxArray& bxs,
                    int             ncomp,
                    int             ngrow,
                    FabAlloc        alloc)
    :
    FabArray<Real,FArrayBox>(bxs,ncomp,ngrow,alloc),
    m_FB_mfcd(0),
    m_FB_fbid(0),
    m_FPB_mfcd(0)
{}

//
// This isn't inlined as it's virtual.
//
MultiFab::~MultiFab()
{
    delete m_FB_mfcd;
    delete m_FB_fbid;
    delete m_FPB_mfcd;
}

void
MultiFab::probe (ostream& os,
                 IntVect& pt)
{
    Real  dat[20];
    int prec = os.precision(14);

    for (MultiFabIterator mfi(*this); mfi.isValid(); ++mfi)
    {
        if (mfi.validbox().contains(pt))
        {
            mfi().getVal(dat,pt);

            os << "point "
               << pt
               << " in box "
               << mfi.validbox()
               << " data = ";
            for (int i = 0, N = mfi().nComp(); i < N; i++)
                os << ' ' << setw(20) << dat[i];
            os << '\n';
        }
    }
    os.precision(prec);

    if (os.fail())
        BoxLib::Error("MultiFab::probe(ostream&,IntVect&) failed");
}

Real
MultiFab::min (int comp,
               int nghost) const
{
    assert(nghost >= 0 && nghost <= n_grow);

    Real mn = INFINITY;

    for (ConstMultiFabIterator mfi(*this); mfi.isValid(false); ++mfi)
    {
        Box b(grow(mfi.validbox(), nghost));
        mn = Min(mn, mfi().min(b, comp));
    }

    ParallelDescriptor::ReduceRealMin(mn);

    return mn;
}

Real
MultiFab::min (const Box& region,
               int        comp,
               int        nghost) const
{
    assert(nghost >= 0 && nghost <= n_grow);

    Real mn = INFINITY;

    for (ConstMultiFabIterator mfi(*this); mfi.isValid(false); ++mfi)
    {
        Box b = ::grow(mfi.validbox(),nghost) & region;

        if (b.ok())
        {
            mn = Min(mn, mfi().min(b, comp));
        }
    }

    ParallelDescriptor::ReduceRealMin(mn);

    return mn;
}

Real
MultiFab::max (int comp,
               int nghost) const
{
    assert(nghost >= 0 && nghost <= n_grow);

    Real mn = -INFINITY;

    for (ConstMultiFabIterator mfi(*this); mfi.isValid(false); ++mfi)
    {
        Box b(grow(mfi.validbox(), nghost));
        mn = Max(mn, mfi().max(b, comp));
    }

    ParallelDescriptor::ReduceRealMax(mn);

    return mn;
}

Real
MultiFab::max (const Box& region,
               int        comp,
               int        nghost) const
{
    assert(nghost >= 0 && nghost <= n_grow);

    Real mn = -INFINITY;

    int first = true;
    for (ConstMultiFabIterator mfi(*this); mfi.isValid(false); ++mfi)
    {
        Box b = ::grow(mfi.validbox(),nghost) & region;

        if (b.ok())
        {
            Real mg = mfi().max(b,comp);
            if (first)
            {
                mn    = mg;
                first = false;
            }
            mn = Max(mn,mg);
        }
    }

    ParallelDescriptor::ReduceRealMax(mn);

    return mn;
}

void
MultiFab::minus (const MultiFab& mf,
                 int             strt_comp,
                 int             num_comp,
                 int             nghost)
{
    assert(boxarray == mf.boxarray);
    assert(strt_comp >= 0);
#ifndef NDEBUG
    int lst_comp = strt_comp + num_comp - 1;
#endif
    assert(lst_comp < n_comp && lst_comp < mf.n_comp);
    assert(nghost <= n_grow && nghost <= mf.n_grow);

    for (MultiFabIterator mfi(*this); mfi.isValid(false); ++mfi)
    {
        DependentMultiFabIterator dmfi(mfi, mf);
        Box bx(mfi.validbox());
        bx.grow(nghost);
        mfi().minus(dmfi(), bx, strt_comp, strt_comp, num_comp);
    }
}

void
MultiFab::plus (Real val,
                int  comp,
                int  num_comp,
                int  nghost)
{
    assert(nghost >= 0 && nghost <= n_grow);
    assert(comp+num_comp <= n_comp);

    for (MultiFabIterator mfi(*this); mfi.isValid(false); ++mfi)
    {
        Box b(grow(mfi.validbox(), nghost));
        mfi().plus(val, b, comp, num_comp);
    }
}

void
MultiFab::plus (Real       val,
                const Box& region,
                int        comp,
                int        num_comp,
                int        nghost)
{
    assert(nghost >= 0 && nghost <= n_grow);
    assert(comp+num_comp <= n_comp);

    for (MultiFabIterator mfi(*this); mfi.isValid(false); ++mfi)
    {
        Box b = ::grow(mfi.validbox(),nghost) & region;

        if (b.ok())
        {
            mfi().plus(val, b, comp, num_comp);
        }
    }
}

void
MultiFab::plus (const MultiFab& mf,
                int             strt_comp,
                int             num_comp,
                int             nghost)
{
    assert(boxarray == mf.boxarray);
    assert(strt_comp >= 0);
#ifndef NDEBUG
    int lst_comp = strt_comp + num_comp - 1;
#endif
    assert(lst_comp < n_comp && lst_comp < mf.n_comp);
    assert(nghost <= n_grow && nghost <= mf.n_grow);

    for (MultiFabIterator mfi(*this); mfi.isValid(false); ++mfi)
    {
        DependentMultiFabIterator dmfi(mfi, mf);
        Box bx(mfi.validbox());
        bx.grow(nghost);
        mfi().plus(dmfi(), bx, strt_comp, strt_comp, num_comp);
    }
}

void
MultiFab::mult (Real val,
                int  comp,
                int  num_comp,
                int  nghost)
{
    assert(nghost >= 0 && nghost <= n_grow);
    assert(comp+num_comp <= n_comp);

    for (MultiFabIterator mfi(*this); mfi.isValid(false); ++mfi)
    {
        Box b(grow(mfi.validbox(), nghost));
        mfi().mult(val, b, comp, num_comp);
    }
}

void
MultiFab::mult (Real       val,
                const Box& region,
                int        comp,
                int        num_comp,
                int        nghost)
{
    assert(nghost >= 0 && nghost <= n_grow);
    assert(comp+num_comp <= n_comp);

    for (MultiFabIterator mfi(*this); mfi.isValid(false); ++mfi)
    {
        Box b = ::grow(mfi.validbox(),nghost) & region;

        if (b.ok())
        {
            mfi().mult(val, b, comp, num_comp);
        }
    }
}

void
MultiFab::invert (Real numerator,
                  int  comp,
                  int  num_comp,
                  int  nghost)
{
    assert(nghost >= 0 && nghost <= n_grow);
    assert(comp+num_comp <= n_comp);

    for (MultiFabIterator mfi(*this); mfi.isValid(false); ++mfi)
    {
        Box b(grow(mfi.validbox(), nghost));
        mfi().invert(numerator, b, comp, num_comp);
    }
}

void
MultiFab::invert (Real       numerator,
                  const Box& region,
                  int        comp,
                  int        num_comp,
                  int        nghost)
{
    assert(nghost >= 0 && nghost <= n_grow);
    assert(comp+num_comp <= n_comp);

    for (MultiFabIterator mfi(*this); mfi.isValid(false); ++mfi)
    {
        Box b = ::grow(mfi.validbox(),nghost) & region;

        if (b.ok())
        {
            mfi().invert(numerator, b, comp, num_comp);
        }
    }
}

void
MultiFab::negate (int comp,
                  int num_comp,
                  int nghost)
{
    assert(nghost >= 0 && nghost <= n_grow);
    assert(comp+num_comp <= n_comp);

    for (MultiFabIterator mfi(*this); mfi.isValid(false); ++mfi)
    {
        Box b(grow(mfi.validbox(), nghost));
        mfi().negate(b, comp, num_comp);
    }
}

void
MultiFab::negate (const Box& region,
                  int        comp,
                  int        num_comp,
                  int        nghost)
{
    assert(nghost >= 0 && nghost <= n_grow);
    assert(comp+num_comp <= n_comp);

    for (MultiFabIterator mfi(*this); mfi.isValid(false); ++mfi)
    {
        Box b = ::grow(mfi.validbox(),nghost) & region;

        if (b.ok())
        {
            mfi().negate(b, comp, num_comp);
        }
    }
}

void
linInterpAddBox (MultiFabCopyDescriptor& fabCopyDesc,
                 BoxList*                returnUnfilledBoxes,
                 Array<FillBoxId>&       returnedFillBoxIds,
                 const Box&              subbox,
                 const MultiFabId&       faid1,
                 const MultiFabId&       faid2,
                 Real                    t1,
                 Real                    t2,
                 Real                    t,
                 int                     src_comp,
                 int                     dest_comp,
                 int                     num_comp,
                 bool                    extrap)
{
    const Real teps = (t2-t1)/1000.0;

    assert(t>t1-teps && (extrap || t < t2+teps));

    if (t < t1+teps)
    {
        returnedFillBoxIds.resize(1);
        returnedFillBoxIds[0] = fabCopyDesc.AddBox(faid1,
                                                   subbox,
                                                   returnUnfilledBoxes,
                                                   src_comp,
                                                   dest_comp,
                                                   num_comp);
    }
    else if (t > t2-teps && t < t2+teps)
    {
        returnedFillBoxIds.resize(1);
        returnedFillBoxIds[0] = fabCopyDesc.AddBox(faid2,
                                                   subbox,
                                                   returnUnfilledBoxes,
                                                   src_comp,
                                                   dest_comp,
                                                   num_comp);
    }
    else
    {
        returnedFillBoxIds.resize(2);
        BoxList tempUnfilledBoxes(subbox.ixType());
        returnedFillBoxIds[0] = fabCopyDesc.AddBox(faid1,
                                                   subbox,
                                                   returnUnfilledBoxes,
                                                   src_comp,
                                                   dest_comp,
                                                   num_comp);
        returnedFillBoxIds[1] = fabCopyDesc.AddBox(faid2,
                                                   subbox,
                                                   &tempUnfilledBoxes,
                                                   src_comp,
                                                   dest_comp,
                                                   num_comp);
        //
        // The boxarrays for faid1 and faid2 should be the
        // same so only use returnUnfilledBoxes from one AddBox here.
        //
    }
}

void
linInterpFillFab (MultiFabCopyDescriptor& fabCopyDesc,
                  const Array<FillBoxId>& fillBoxIds,
                  const MultiFabId&       faid1,
                  const MultiFabId&       faid2,
                  FArrayBox&              dest,
                  Real                    t1,
                  Real                    t2,
                  Real                    t,
                  int                     src_comp,   // these comps need to be removed
                  int                     dest_comp,  // from this routine
                  int                     num_comp,
                  bool                    extrap)
{
    const Real teps = (t2-t1)/1000.0;

    assert(t>t1-teps && (extrap || t < t2+teps));

    if (t < t1+teps)
    {
        fabCopyDesc.FillFab(faid1, fillBoxIds[0], dest);
    }
    else if (t > t2-teps && t < t2+teps)
    {
        fabCopyDesc.FillFab(faid2, fillBoxIds[0], dest);
    }
    else
    {
        assert(dest_comp + num_comp <= dest.nComp());

        FArrayBox dest1(dest.box(), dest.nComp());
        dest1.setVal(1.e30);
        FArrayBox dest2(dest.box(), dest.nComp());
        dest2.setVal(1.e30);
        fabCopyDesc.FillFab(faid1, fillBoxIds[0], dest1);
        fabCopyDesc.FillFab(faid2, fillBoxIds[1], dest2);
        dest.linInterp(dest1,
                       dest1.box(),
                       src_comp,
                       dest2,
                       dest2.box(),
                       src_comp,
                       t1,
                       t2,
                       t,
                       dest.box(),
                       dest_comp,
                       num_comp);
    }
}

void
MultiFab::buildFBmfcd (int start_comp,
                       int num_comp)
{
    assert(m_FB_mfcd == 0);

    m_FB_mfcd = new MultiFabCopyDescriptor;

    MultiFabId mfid = m_FB_mfcd->RegisterMultiFab(this);

    assert(mfid == MultiFabId(0));

    m_FB_fbid = new vector<FillBoxId>;

    for (MultiFabIterator mfi(*this); mfi.isValid(false); ++mfi)
    {
        for (int j = 0; j < length(); j++)
        {
            if (j == mfi.index())
                //
                // Don't copy into self.
                //
                continue;

            if (boxarray[j].intersects(mfi().box()))
            {
                Box bx = boxarray[j] & mfi().box();

                m_FB_fbid->push_back(m_FB_mfcd->AddBox(mfid,
                                                       bx,
                                                       0,
                                                       j,
                                                       start_comp,
                                                       start_comp,
                                                       num_comp));

                m_FB_fbid->back().FabIndex(mfi.index());
            }
        }
    }
}

void
MultiFab::FillBoundary (int start_comp,
                        int num_comp)
{
    RunStats fill_boundary_stats("fill_boundary");

    fill_boundary_stats.start();

    MultiFabCopyDescriptor& mfcd = theFBmfcd(start_comp,num_comp);

    const vector<FillBoxId>& fillBoxIDs = theFBfbid();

    mfcd.CollectData();

    const int MyProc = ParallelDescriptor::MyProc();

    const MultiFabId TheFBMultiFabId = 0;

    for (int i = 0; i < fillBoxIDs.size(); i++)
    {
        int fabindex = fillBoxIDs[i].FabIndex();

        assert(ProcessorMap()[fabindex] == MyProc);
        //
        // Directly fill the FAB.
        //
        mfcd.FillFab(TheFBMultiFabId, fillBoxIDs[i], (*this)[fabindex]);
    }

    fill_boundary_stats.end();
}

void
MultiFab::buildFPBmfcd (const Geometry& geom,
                        int             src_comp,
                        int             num_comp,
                        bool            no_ovlp)
{
    assert(m_FPB_mfcd == 0);
    //
    // Get list of intersection boxes.
    //
    Geometry::PIRMMap& pirm = geom.getPIRMMap(boxArray(),nGrow(),no_ovlp);
    //
    // Fill intersection list.
    //
    // Assumes PIRec MultiMap built correctly (i.e. each entry indexed on
    // local fab id, contains srcBox/dstBox pairs to copy valid data from
    // other fabs in the array).  Assumes box-pairs constructed so that
    // all data is "fillable" from the valid region of "fa".
    //
    m_FPB_mfcd = new MultiFabCopyDescriptor;

    MultiFabId mfid = m_FPB_mfcd->RegisterMultiFab(this);

    assert(mfid == MultiFabId(0));

    typedef Geometry::PIRMMap::iterator PIRMMapIt;
    //
    // Register boxes in copy descriptor.
    //
    for (PIRMMapIt p_it = pirm.begin(); p_it != pirm.end(); ++p_it)
    {
        (*p_it).second.fbid = m_FPB_mfcd->AddBox(mfid,
                                                 (*p_it).second.srcBox,
                                                 0,
                                                 (*p_it).second.srcId,
                                                 src_comp,
                                                 src_comp,
                                                 num_comp);
    }
}
