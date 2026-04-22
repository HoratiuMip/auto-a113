#include <bridge.hpp>
using namespace mdn;

#define DOCK_NAME "hon-opt-alpha"

using namespace glm;

struct Dock : dock_t {
public:
    Dock( void ) = default;

public:
    inline static const char* const METHODS[] = {
        "Newt",
        "Steep",
        "Conj-FR",
        "Conj-PR",
        "Conj-HS",
        "QNewt-DFP",
        "QNewt-BFGS"
    };
    inline static constexpr int METHOD_COUNT = sizeof( METHODS ) / sizeof( const char* );
    enum Method_ {
        Method_Newton,
        Method_Steepest,
        Method_ConjugateFr,
        Method_ConjugatePr,
        Method_ConjugateHs,
        Method_QuasiNewtonDFP,
        Method_QuasiNewtonBFGS
    };

public:
    struct _ex_t {
        _ex_t( void ) = default;

         _ex_t( std::string_view in_exp_ )
        : exp{ in_exp_ }
        {
            f = [ this ] ( double x1, double x2 ) {
                exp.bind( [ x1, x2 ] ( std::string_view var_, double* val_ ) -> status_t {
                    if( var_ == "x1" ) { *val_ = x1; return A113_OK; }
                    if( var_ == "x2" ) { *val_ = x2; return A113_OK; }
                    return A113_ERR_NOT_FOUND;
                } );
                double res = 0.0; 
                MDN_ASSERT_OR( A113_OK == exp.resolve( &res ) && not std::isnan( res ) ) return 0.0;
                return res;
            };

            constexpr double step = 0.1;
            grid.span_s( { { step, -8, 8 }, { step, 8, -8 } } );
            grid.apply( [ this ] ( double* x ) -> double { return f( x[0], x[1] ); } );
        }

        text::Fastexp< double >   exp;
        mdn_0::fnc_2d_t<>         f;
        mdn_2::grid_t<>           grid;
    };

public:
    Dispenser< _ex_t >   ex                           = { DispenserMode_Drop };    

    std::string          in_exp;

    int                  step_count[ METHOD_COUNT ]   = { 0 };
    int                  all_step_count               = 0;

    mdn_0::arr_t<2>      grad;
    mdn_0::arr_t<4>      hess;
    mdn_0::arr_t<2>      x0                           = { 0, 0 };

public:
    void compute_gradient( mdn_0::arr_t<2> X ) { grad = mdn_0::d1_f2( X[0], X[1], 1e-6, ex->f ); }

    void compute_hessian( mdn_0::arr_t<2> X ) { hess = mdn_0::d2h_f2( X[0], X[1], 1e-6, ex->f ); }

public:
    MDN_DOCK_NAME_FNC override { return DOCK_NAME; }

    MDN_DOCK_GUIX_FNC override {
        auto ex = this->ex.watch();

        bool open = true;
        if( ImGui::Begin( "Optimizations Alpha", &open, ImGuiWindowFlags_None ) ) {
            ImGui::Separator();
            ImGui::TextUnformatted( "f(x1,x2) =" ); ImGui::SameLine();
            const bool new_exp = ImGui::InputText( "##in-exp", &in_exp, ImGuiInputTextFlags_EnterReturnsTrue );
            ImGui::Separator();

            if( ex ) {
                ImPlot::PushColormap( ImPlotColormap_Viridis );
                if( ImPlot::BeginPlot( "##plt-f", {680, 680} ) ) {
                    if( ImPlot::IsPlotHovered() && ImGui::IsMouseClicked( ImGuiMouseButton_Left ) && ImGui::IsKeyDown( ImGuiKey_LeftCtrl ) ) {
                        auto p = ImPlot::GetPlotMousePos();
                        x0 = { p.x, p.y };
                    }
                    
                    ImPlot::PlotHeatmap(
                        "##htm-f", ex->grid.raw(), ex->grid.n_of(1), ex->grid.n_of(0),
                        ex->grid.min(), ex->grid.max(), nullptr, {-8,-8}, {8,8},
                        ImPlotHeatmapFlags_None
                    );

                    ImPlot::PushColormap( ImPlotColormap_Twilight );
                    ImPlot::PushStyleVar( ImPlotStyleVar_LineWeight, 3.0f );
                    ImPlot::PushStyleColor( ImPlotCol_MarkerFill, ImVec4{ 1,1,1,1 } );

                    /* === Newton === */ {
                    auto xk = x0;
                    for( int n = 1; n <= step_count[ Method_Newton ]; ++n ) {
                        compute_gradient( xk ); 
                        compute_hessian( xk );
                        MDN_ASSERT_OR( A113_OK == mdn_0::invm( hess.get(), 2 ) ) break;

                        mdn_0::arr_t<2> xk1 = {
                            xk.x() - hess[0]*grad.x() - hess[1]*grad.y(),
                            xk.y() - hess[2]*grad.x() - hess[3]*grad.y()
                        };

                        ImPlot::PlotLine( METHODS[ Method_Newton ], (double[2]){ xk.x(), xk1.x() }, (double[2]){ xk.y(), xk1.y() }, 2 );
                        if( ImPlot::IsLegendEntryHovered( METHODS[ Method_Newton ] ) ) {
                            ImPlot::PlotScatter( "", (double[2]){ xk.x(), xk1.x() }, (double[2]){ xk.y(), xk1.y() }, 2 );
                        }

                        xk = xk1;
                    }
                    }

                    /* === Steepest === */ {
                    auto xk = x0;
                    for( int n = 1; n <= step_count[ Method_Steepest ]; ++n ) {
                        compute_gradient( xk );
                        auto dk = -grad; dk /= dk.norm();
                        
                        auto [ s, _ ] = mdn_0::search_mf1_elimgr< double >( 0.0, 1.0, 0.00001, nullptr, [ & ] ( double s_ ) -> double {
                            auto dxk = xk + dk*s_; return ex->f( dxk.x(), dxk.y() );
                        } );

                        auto xk1 = xk + dk*s;

                        ImPlot::PlotLine( METHODS[ Method_Steepest ], (double[2]){ xk.x(), xk1.x() }, (double[2]){ xk.y(), xk1.y() }, 2 );
                        if( ImPlot::IsLegendEntryHovered( METHODS[ Method_Steepest ] ) ) {
                            ImPlot::PlotScatter( "", (double[2]){ xk.x(), xk1.x() }, (double[2]){ xk.y(), xk1.y() }, 2 );
                        }

                        xk = xk1;
                    }
                    }

                    /* === Conjugate FR === */ {
                    auto             xk = x0;
                    mdn_0::arr_t<2>  dk = { 0, 0 };
                    double           bk = 0;
                    compute_gradient( xk );
                    for( int n = 1; n <= step_count[ Method_ConjugateFr ]; ++n ) {
                        auto gk = grad;
                        
                        dk = -gk + dk*bk;

                        auto [ s, _ ] = mdn_0::search_mf1_elimgr< double >( 0.0, 1.0, 0.00001, nullptr, [ & ] ( double s_ ) -> double {
                            auto dxk = xk + dk*s_; return ex->f( dxk.x(), dxk.y() );
                        } );

                        auto xk1 = xk + dk*s;

                        ImPlot::PlotLine( METHODS[ Method_ConjugateFr ], (double[2]){ xk.x(), xk1.x() }, (double[2]){ xk.y(), xk1.y() }, 2 );
                        if( ImPlot::IsLegendEntryHovered( METHODS[ Method_ConjugateFr ] ) ) {
                            ImPlot::PlotScatter( "", (double[2]){ xk.x(), xk1.x() }, (double[2]){ xk.y(), xk1.y() }, 2 );
                        }

                        compute_gradient( xk1 );
                        auto gk1 = grad;
                        
                        bk = pow(gk1.norm(),2) / pow(gk.norm(),2);

                        xk = xk1;
                    }
                    }

                    /* === Conjugate PR === */ {
                    auto             xk = x0;
                    mdn_0::arr_t<2>  dk = { 0, 0 };
                    double           bk = 0;
                    compute_gradient( xk );
                    for( int n = 1; n <= step_count[ Method_ConjugatePr ]; ++n ) {
                        auto gk = grad;
                        
                        dk = -gk + dk*bk;

                        auto [ s, _ ] = mdn_0::search_mf1_elimgr< double >( 0.0, 1.0, 0.00001, nullptr, [ & ] ( double s_ ) -> double {
                            auto dxk = xk + dk*s_; return ex->f( dxk.x(), dxk.y() );
                        } );

                        auto xk1 = xk + dk*s;

                        ImPlot::PlotLine( METHODS[ Method_ConjugatePr ], (double[2]){ xk.x(), xk1.x() }, (double[2]){ xk.y(), xk1.y() }, 2 );
                        if( ImPlot::IsLegendEntryHovered( METHODS[ Method_ConjugatePr ] ) ) {
                            ImPlot::PlotScatter( "", (double[2]){ xk.x(), xk1.x() }, (double[2]){ xk.y(), xk1.y() }, 2 );
                        }

                        compute_gradient( xk1 );
                        auto gk1 = grad;

                        bk = gk1.dot( gk1-gk ) / gk.dot( gk );

                        xk = xk1;
                    }
                    }

                    /* === Conjugate HS === */ {
                    auto             xk = x0;
                    mdn_0::arr_t<2>  dk = { 0, 0 };
                    double           bk = 0;
                    compute_gradient( xk );
                    for( int n = 1; n <= step_count[ Method_ConjugateHs ]; ++n ) {
                        auto gk = grad;
                        
                        dk = -gk + dk*bk;

                        auto [ s, _ ] = mdn_0::search_mf1_elimgr< double >( 0.0, 1.0, 0.00001, nullptr, [ & ] ( double s_ ) -> double {
                            auto dxk = xk + dk*s_; return ex->f( dxk.x(), dxk.y() );
                        } );

                        auto xk1 = xk + dk*s;

                        ImPlot::PlotLine( METHODS[ Method_ConjugateHs ], (double[2]){ xk.x(), xk1.x() }, (double[2]){ xk.y(), xk1.y() }, 2 );
                        if( ImPlot::IsLegendEntryHovered( METHODS[ Method_ConjugateHs ] ) ) {
                            ImPlot::PlotScatter( "", (double[2]){ xk.x(), xk1.x() }, (double[2]){ xk.y(), xk1.y() }, 2 );
                        }

                        compute_gradient( xk1 );
                        auto gk1 = grad;

                        bk = gk1.dot( gk1-gk ) / dk.dot( gk1-gk );

                        xk = xk1;
                    }
                    }

                    /* === Quasi Newton DFP === */ {
                    auto            xk = x0;
                    double          k  = 0;
                    mdn_0::arr_t<4> B  = { 1,0, 0,1 };
                    compute_gradient( xk );
                    for( int n = 1; n <= step_count[ Method_QuasiNewtonDFP ]; ++n ) {
                        mdn_0::arr_t<2> dk = {
                            -B[0]*grad[0] - B[1]*grad[1],
                            -B[2]*grad[0] - B[3]*grad[1]
                        };

                        auto [ s, _ ] = mdn_0::search_mf1_elimgr< double >( 0.0, 1.0, 0.00001, nullptr, [ & ] ( double s_ ) -> double {
                            auto dxk = xk + dk*s_; return ex->f( dxk[0], dxk[1] );
                        } );

                        auto xk1 = xk + dk*s;

                        ImPlot::PlotLine( METHODS[ Method_QuasiNewtonDFP ], (double[2]){ xk.x(), xk1.x() }, (double[2]){ xk.y(), xk1.y() }, 2 );
                        if( ImPlot::IsLegendEntryHovered( METHODS[ Method_QuasiNewtonDFP ] ) ) {
                            ImPlot::PlotScatter( "", (double[2]){ xk.x(), xk1.x() }, (double[2]){ xk.y(), xk1.y() }, 2 );
                        }


                        auto gk = grad;
                        compute_gradient( xk1 );
                        auto gk1 = grad;

                        auto dxk = xk1 - xk; 
                        auto Gk  = gk1 - gk;

                        mdn_0::arr_t<4> a = {
                            dxk[0]*dxk[0], dxk[0]*dxk[1],
                            dxk[1]*dxk[0], dxk[1]*dxk[1]
                        };
                        a /= dxk.dot(Gk);

                        mdn_0::arr_t<2> b = {
                            B[0]*Gk[0] + B[1]*Gk[1], B[2]*Gk[0] + B[3]*Gk[1]
                        };

                        mdn_0::arr_t<4> c = {
                            b[0]*b[0], b[0]*b[1],
                            b[1]*b[0], b[1]*b[1]
                        };
                        c /= Gk.dot(b);

                        B = B + a - c;
                        xk = xk1;
                    }
                    }

                    /* === Quasi Newton BFGS === */ {
                    auto            xk = x0;
                    double          k  = 0;
                    mdn_0::arr_t<4> B  = { 1,0, 0,1 };
                    compute_gradient( xk );
                    for( int n = 1; n <= step_count[ Method_QuasiNewtonBFGS ]; ++n ) {
                        mdn_0::arr_t<2> dk = {
                            -B[0]*grad[0] - B[1]*grad[1],
                            -B[2]*grad[0] - B[3]*grad[1]
                        };

                        auto [ s, _ ] = mdn_0::search_mf1_elimgr< double >( 0.0, 1.0, 0.00001, nullptr, [ & ] ( double s_ ) -> double {
                            auto dxk = xk + dk*s_; return ex->f( dxk[0], dxk[1] );
                        } );

                        auto xk1 = xk + dk*s;

                        ImPlot::PlotLine( METHODS[ Method_QuasiNewtonBFGS ], (double[2]){ xk.x(), xk1.x() }, (double[2]){ xk.y(), xk1.y() }, 2 );
                        if( ImPlot::IsLegendEntryHovered( METHODS[ Method_QuasiNewtonBFGS ] ) ) {
                            ImPlot::PlotScatter( "", (double[2]){ xk.x(), xk1.x() }, (double[2]){ xk.y(), xk1.y() }, 2 );
                        }


                        auto gk = grad;
                        compute_gradient( xk1 );
                        auto gk1 = grad;

                        auto dxk = xk1 - xk; 
                        auto Gk  = gk1 - gk;

                        mdn_0::arr_t<4> a = {
                            Gk[0]*Gk[0], Gk[0]*Gk[1],
                            Gk[1]*Gk[0], Gk[1]*Gk[1]
                        };
                        a /= Gk.dot(dxk);

                        mdn_0::arr_t<2> b = {
                            B[0]*dxk[0] + B[1]*dxk[1], B[2]*dxk[0] + B[3]*dxk[1]
                        };

                        mdn_0::arr_t<4> c = {
                            b[0]*b[0], b[0]*b[1],
                            b[1]*b[0], b[1]*b[1]
                        };
                        c /= dxk.dot(b);

                        B = B + a - c;
                        xk = xk1;
                    }
                    }

                    ImPlot::PopStyleColor( 1 );
                    ImPlot::PopStyleVar( 1 );
                    ImPlot::PopColormap( 1 );
                    ImPlot::EndPlot();
                }

                ImGui::SameLine();
                ImPlot::ColormapScale( "z", ex->grid.min(), ex->grid.max(), {100,680} );
                ImPlot::PopColormap(); 
                
                ImGui::SameLine();
                ImGui::BeginTable( "Steps", 1+METHOD_COUNT, ImGuiTableFlags_BordersOuter | ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_HighlightHoveredColumn );
                for( int i = 0x0; i < METHOD_COUNT; ++i ) {
                    ImGui::TableSetupColumn( METHODS[i], ImGuiTableColumnFlags_AngledHeader );
                }
                ImGui::TableSetupColumn( "##tblcol-all", ImGuiTableColumnFlags_None );

                ImGui::TableAngledHeadersRow();
                ImGui::TableNextRow();
                
                for( int i = 0x0; i < METHOD_COUNT; ++i ) {
                    ImGui::TableNextColumn();
                    ImGui::VSliderInt( std::format( "##in-step-count-{}", METHODS[i] ).c_str(), {36, 590}, step_count + i, 0, 100 );
                }
                ImGui::TableNextColumn();
                if( ImGui::VSliderInt( "##in-step-count-all", {36, 590}, &all_step_count, 0, 100 ) ) {
                    std::fill_n( step_count, METHOD_COUNT, all_step_count );
                }

                ImGui::EndTable();

                ImGui::SeparatorText( "Initial guess" );
                ImGui::SetNextItemWidth( 680 );
                ImGui::InputScalarN( "##in-ig", ImGuiDataType_Double, &x0, 2 );
            }

            if( new_exp ) {
                std::thread( [ this ] { if( not in_exp.empty() ) this->ex.control( in_exp ); } ).detach();
            }
        } 
        ImGui::End();
        return open ? A113_OK : A113_ERR_TERMINATED; 
    }
};

struct Proxy : proxy_t {
public:
    Proxy( void )
    : proxy_t{
        DOCK_NAME,
        {},
        { 
        MDN_PROXY_CLI_BASIC_INSTALL
        }
    } {}
};

MDN_AUTO_INSTALL( Proxy );
